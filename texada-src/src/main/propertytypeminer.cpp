/*
 * propertytypeminer.cpp
 *
 *  Created on: Jun 3, 2014
 *      Author: clemieux
 */

#include "propertytypeminer.h"

#include <fstream>
#include <assert.h>

#include <ltlparse/public.hh>
#include <ltlvisit/apcollect.hh>
#include <ltlvisit/tostring.hh>

#include <boost/program_options.hpp>

#include "../parsers/parser.h"
#include "../parsers/linearparser.h"
#include "../parsers/mapparser.h"
#include "../parsers/prefixtreeparser.h"
#include "../instantiation-tools/constinstantspool.h"
#include "../instantiation-tools/pregeninstantspool.h"
#include "../instantiation-tools/otfinstantspool.h"
#include "../instantiation-tools/instantspoolcreator.h"
#include "../checkers/maptracechecker.h"
#include "../checkers/lineartracechecker.h"
#include "../checkers/prefixtreechecker.h"
#include "../checkers/statistic.h"
#include "../checkers/finding.h"
#include "../instantiation-tools/apsubbingcloner.h"
#include "opts.h"

namespace texada {

/**
 * Finds instantiations of the given property type using the linear
 * trace checker. NOTE: this method is only used in testing.
 * @param formula_string the string form of the property type/
 * ltl formula to be mined
 * @param trace_source the input source of the trace
 * @return valid instantiations of the property type on the trace
 */
set<std::pair<const spot::ltl::formula*, statistic>> mine_lin_property_type(string formula_string,
        string trace_source) {
    return mine_property_type(
            set_options("-f '" + formula_string + "' -l " + trace_source));
}

/**
 * Finds instantiations of the given property type using the map
 * trace checker. NOTE: this method is only used in testing.
 * @param formula_string the string form of the property type/
 * ltl formula to be mined
 * @param trace_source the input source of the trace
 * @return valid instantiations of the property type on the trace
 */
set<std::pair<const spot::ltl::formula*, statistic>> mine_map_property_type(string formula_string,
        string trace_source) {
    return mine_property_type(
            set_options("-f '" + formula_string + "' -m " + trace_source));
}

/**
 * Finds valid instantiations of the given property type on the given
 * traces, using map or linear checker according to use_map.
 * @param formula_string the string form of the property type/
 * ltl formula to be mined
 * @param trace_source the input source of the trace
 * @param use_map use map miner if true, linear miner otherwise
 * @return valid instantiations of the inputted formula on inputted trace set
 */
set<std::pair<const spot::ltl::formula*, statistic>> mine_property_type(                                  // Dennis: need to change return type to return support and confidence information
        boost::program_options::variables_map opts) {
    // collect all relevant information from options
    // what trace type to use. TODO: update for pre_tree when added
    bool use_map = opts.count("map-trace");
    bool use_lin = opts.count("linear-trace");
    bool use_pretree = opts.count("prefix-tree-trace");
    // whether to allow repetition in instantiations
    bool allow_reps = opts.count("allow-same-bindings");
    // whether to pregenerate instantiations
    bool pregen_instants = opts.count("pregen-instants");
    // whether to output finding support
    bool show_sup = opts.count("support");
    // the confidence threshold
    int conf_threshold;
    if (opts.count("confidence")) {
        conf_threshold = opts["confidence"].as<int>();
    } else {
        conf_threshold = 100;
    }
    // check that confidence threshold is within range
    if (conf_threshold < 0 || conf_threshold > 100) {
        std::cerr << "conf_threshold must be between 0 to 100 \n";
        exit(1);
    }
    // the property type
    string prop_type = opts["property-type"].as<std::string>();
    // trace source
    string trace_source = opts["log-file"].as<std::string>();

    // if any constant events are specified in command line,
    // take these into count
    vector<string> specified_formula_events;
    if (opts.count("event")) {
        specified_formula_events = opts["event"].as<vector<string>>();
    }

    //parse the ltl formula
    spot::ltl::parse_error_list parse_errs;
    const spot::ltl::formula* formula = spot::ltl::parse(prop_type, parse_errs);
    assert(parse_errs.size() == 0);

    // parse file
    std::ifstream infile(trace_source);
    if (infile.fail()) {
        std::cerr << "Error: could not open file \n";
        return set<std::pair<const spot::ltl::formula*, statistic>>();
    }

    parser * parser;
    if (use_map) {
        parser = new map_parser();
    } else if (use_lin) {
        parser = new linear_parser();
    } else if (use_pretree) {
        parser = new prefix_tree_parser();
    } else {
        std::cerr << "Trace type not specified, nothing gets parsed.";
        exit(1);
    }
    // configure parser based on user options
    if (opts.count("regex")) {
        parser->set_event_types(opts["regex"].as<vector<string>>());
    }
    if (opts.count("separator_regex")) {
        parser->set_separator(opts["separator_regex"].as<std::string>());
    }
    if (opts.count("ignore_nm_lines")) {
        parser->ignore_nm_lines();
    }
    parser->parse(infile);

    shared_ptr<set<string>> event_set = parser->return_events();
    // if we don't want repetition and there are already events
    // in the formula, we can just exclude them from the event set
    // to start with
    if (!allow_reps && (specified_formula_events.size() > 0)) {
        for (int i = 0; i < specified_formula_events.size(); i++) {
            event_set->erase(specified_formula_events.at(i));
        }
    }

    // create the set of formula's variables
    shared_ptr<spot::ltl::atomic_prop_set> variables(
            spot::ltl::atomic_prop_collect(formula));
    // remove variables which are specified as constant events
    if (specified_formula_events.size() > 0) {
        spot::ltl::atomic_prop_set::iterator it = variables->begin();
        while (it != variables->end()) {
            bool erase = false;
            for (int i = 0; i < specified_formula_events.size(); i++) {
                if ((*it)->name() == specified_formula_events.at(i)) {
                    erase = true;
                }
            }
            if (erase) {
                spot::ltl::atomic_prop_set::iterator toErase = it;
                ++it;
                variables->erase(toErase);
            } else {
                ++it;
            }
        }
    }

    // create the instantiator
    instants_pool_creator * instantiator;

    if (variables->empty()) {
        instantiator = new const_instants_pool(formula);
    } else if (pregen_instants) {
        instantiator = new pregen_instants_pool(event_set, variables,
                allow_reps, specified_formula_events);
    } else {
        instantiator = new otf_instants_pool(event_set, variables, allow_reps,
                specified_formula_events);
    }

    vector<finding> valid_instants;                                 // Dennis: need to change type to accept sup and conf info
    // check all valid instantiations on each trace
    if (use_lin) {
        shared_ptr<set<vector<string_event> >> vector_trace_set =
                dynamic_cast<linear_parser*>(parser)->return_vec_trace();
        valid_instants = valid_instants_on_traces(formula, instantiator,        // Dennis: need to change signature to input conf threshold
                vector_trace_set, conf_threshold);
    } else if (use_map) {
        shared_ptr<set<map<string_event, vector<long>>> > map_trace_set = dynamic_cast<map_parser*>(parser)->return_map_trace();
        valid_instants = valid_instants_on_traces(formula, instantiator,        // Dennis: need to change signature to input conf threshold
                map_trace_set);
    } else if (use_pretree) {
        shared_ptr<prefix_tree> prefix_tree_traces = dynamic_cast<prefix_tree_parser*>(parser)->return_prefix_trees();
        valid_instants = valid_instants_on_traces(formula, instantiator,        // Dennis: need to change signature to input conf threshold
                prefix_tree_traces);
    }

    set<std::pair<const spot::ltl::formula*, statistic>> return_set;                                  // Dennis: need to change type to carry sup and conf info
    int valid_instants_size = valid_instants.size();
    for (int i = 0; i < valid_instants_size; i++) {
        map<string, string> binding_i = valid_instants.at(i).binding;
        statistic stat_i = valid_instants.at(i).stat;
        for (map<string, string>::iterator it = binding_i.begin();   // Dennis: need to change type
                it != binding_i.end(); it++) {
        }
        const spot::ltl::formula * valid_form_i = instantiate(formula,
                binding_i, specified_formula_events);                // Dennis: need to also take out sup and conf info
        return_set.insert(std::make_pair(valid_form_i, stat_i));
    }

    delete instantiator;
    delete parser;
    formula->destroy();
    return return_set;

}

} /* namespace texada */
