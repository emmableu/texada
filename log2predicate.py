output = open('output.txt', 'r')
lines = output.readlines()
predicate_list = []
last_line = ""
for line in lines:
    line = line.rstrip()
    if line == "# project: sample.sb3":
        continue
    elif line == "--":
        if not predicate_list:
            continue
        else:
            predicate_list.append(["next_round", "", ""])
            last_line = ""
            continue
    elif line == last_line:
        continue
    elif line != last_line and len(line.split(" "))>1:
        if not last_line:
            predicate_list.append(["start", line.split(" ")[-1], line])
        else:
            data_list = [data.split(":")[1] for data in line.split(" ")]
            x, y, input = [int(data_list[0]), int(data_list[1]), data_list[2]]
            print("last_line: ", last_line)
            last_line_list = [data.split(":")[1] for data in last_line.split(" ")]
            last_x, last_y, last_input = [int(last_line_list[0]), int(last_line_list[1]), last_line_list[2]]
            # if input is not the same as the last_input, then there must be a new entry in predicate_list
            if input != last_input and x == last_x and y == last_y:
                predicate_list.append(["stop", line.split(" ")[-1], line])
#            if input x is not the same as the last_input, then should check the last entry in predicate_list
#            if it's move_left, then if x < last_x, no need to add in new entry.
            if x != last_x and y == last_y and input == last_input:
                moving_predicates = ['move_left', 'move_right']
                if predicate_list[-1][0] not in moving_predicates:
                    left_or_right = int(x - last_x > 0)
                    predicate_list.append([moving_predicates[left_or_right], line.split(" ")[-1], line])
    last_line = line

import pandas as pd
df = pd.DataFrame(predicate_list)
df.to_csv("predicate_list.csv")

basic_log = []
for line in predicate_list:
    if line[0] == "next_round":
        basic_log.append("--\n")
        continue
    basic_log.append(line[1] + '\n' + line[0] + "\n..\n")


file = open('basic_log.txt', 'w')
file.writelines(basic_log)
file.close()












