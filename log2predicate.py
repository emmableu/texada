import json
import pandas as pd
output = open('output.json', 'r')
data = json.load(output)
# print(json.dumps(data[0:4], indent=4, separators=(',', ': ')))
all_sprites = set([d['sprite']['name'] for d in data])
predicate_list_dict = {}

for sprite in all_sprites:
    predicate_list_dict[sprite] = pd.DataFrame(columns = ['timestamp', 'motion', 'x', 'y', 'touching', 'block', 'keysDown', 'variables', 'stageVariables'])

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


for d in data:
    sprite = d['sprite']['name']
    sprite_df = predicate_list_dict[sprite]

    stage_variable = d['stageVariables']
    simplified_variables = {}
    for variable in stage_variable.keys():
        if variable not in stage_var_dict:
            stage_var_dict[variable] = "var" + str(len(stage_var_dict) + 1)
        simplified_variables[stage_var_dict[variable]] = stage_variable[variable]['value']

    sprite_df.loc[len(sprite_df)] = {
        "timestamp": d['clockTime'],
        "motion": "motion",
        'x': d["sprite"]['x'],
        'y': d["sprite"]['y'],
        'touching': d["sprite"]["touching"],
        'block': block_simplify(d['block']),
        'keysDown': d['keysDown'],
        'variables': d['sprite']['variables'],
        'stageVariables': simplified_variables
    }

for sprite in all_sprites:
    df = pd.DataFrame(predicate_list_dict[sprite])
    df.to_csv("output/" + sprite + ".csv")


