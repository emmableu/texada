import json
import pandas as pd
output = open('output.json', 'r')
data = json.load(output)
# print(json.dumps(data[0:4], indent=4, separators=(',', ': ')))
all_sprites = set([d['sprite']['name'] for d in data])
predicate_list_dict = {}

for sprite in all_sprites:
    predicate_list_dict[sprite] = pd.DataFrame(columns = ['clockTime', 'motion', 'x', 'y', 'touching', 'block', 'keysDown', 'variables', 'stageVariables'])

complete_df = pd.DataFrame(columns = ['clockTime', 'sprite', 'motion', 'x', 'y', 'touching', 'block', 'keysDown', 'variables', 'stageVariables'])

stage_var_dict = {}


def block_simplify(block):
    if "opcode" not in block:
        return block
    opcode = block['opcode']
    simple_field = ""
    if block['fields']:
        field = block['fields']
        field_key = list(field.keys())[0]
        value = field[field_key]['value']
        simple_field = "(" + field_key + ":" + str(value) + ")"
    if not block['inputs']:
        return opcode + simple_field
    else:
        sub_blocks = ""
        for input_block in block['inputs']:
            simple_sub_block = block_simplify(input_block)
            sub_blocks += simple_sub_block
        simple_block = opcode + simple_field + "{" + sub_blocks + "}"
        return simple_block


def variable_simplify(variable, stage_var_dict):
    simple_variables = {}
    for v in variable.keys():
        if v not in stage_var_dict:
            stage_var_dict[v] = variable[v]['name']
        simple_variables[stage_var_dict[v]] = variable[v]['value']
    return simple_variables

def get_data_per_sprite():
    for d in data:
        sprite = d['sprite']['name']
        sprite_df = predicate_list_dict[sprite]
        sprite_df.loc[len(sprite_df)] = {
            "clockTime": d['clockTime'],
            "motion": "motion",
            'x': d["sprite"]['x'],
            'y': d["sprite"]['y'],
            'touching': d["sprite"]["touching"],
            'block': block_simplify(d['block']),
            'keysDown': d['keysDown'],
            'variables': variable_simplify(d['sprite']['variables'], stage_var_dict),
            'stageVariables': variable_simplify(stage_variable, stage_var_dict)
        }
    for sprite in all_sprites:
        df = pd.DataFrame(predicate_list_dict[sprite])
        df.to_csv("output/" + sprite + ".csv")


def get_data():
    for d in data:
        complete_df.loc[len(complete_df)] = {
            "clockTime": d['clockTime'],
            "sprite": d['sprite']['name'],
            "motion": "motion",
            'x': d["sprite"]['x'],
            'y': d["sprite"]['y'],
            'touching': d["sprite"]["touching"],
            'block': block_simplify(d['block']),
            'keysDown': d['keysDown'],
            'variables': variable_simplify(d['sprite']['variables'], stage_var_dict),
            'stageVariables': variable_simplify(d['stageVariables'], stage_var_dict)
        }
        df = pd.DataFrame(complete_df)
        df.to_csv("output/" + "complete" + ".csv")


get_data()