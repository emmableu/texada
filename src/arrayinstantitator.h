/*
 * arrayinstantitator.h
 *
 *  Created on: May 21, 2014
 *      Author: clemieux
 */

#ifndef ARRAYINSTANTITATOR_H_
#define ARRAYINSTANTITATOR_H_

#include <set>
#include <string>
#include <map>
#include <ltlast/allnodes.hh>

namespace texada {

/**
 * class which creates an array of instantiating functions (i.e. maps) given
 * the number of variables in an LTL formula and a set of events from traces.
 */
class array_instantiator {
public:
	array_instantiator(std::set<std::string>* events,
			std::set<const spot::ltl::atomic_prop*>*);
	virtual ~array_instantiator();
	void traverse_and_fill(std::string event, int i, int k);
	struct inst_fxn{
		std::map<std::string, std::string> inst_map;
		bool validity;
	};
	inst_fxn* return_instantiations();
private:
	inst_fxn* return_array;
	std::set<const spot::ltl::atomic_prop*>* formula_vars;
	std::set<std::string>* events;
	int length;
};

} /* namespace texada */

#endif /* ARRAYINSTANTITATOR_H_ */
