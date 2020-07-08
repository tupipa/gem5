#!/usr/bin/python3


import sys
import os

#############################################
# parse a stats.txt from gem5/m5out/
# filter out the interested fields defined in
# the list of
#
#    filter_records = [ 'key1', 'key2' , ...]
#
# result will print to stdout
def usage():
    print ('Usage: filter-log.py <stats.txt>')


filter_records = []

mem_ctrls = [
    'system.mem_ctrls.num_reads::total',
    'system.mem_ctrls.readReqs',
    'system.mem_ctrls.writeReqs'
]

membus = [
    'system.membus.pkt_count::total',
    'system.membus.pkt_size::total'
]

tgen = [
    'system.tgen.totalReads',
    'system.tgen.totalWrites',
    'system.tgen.avgReadLatency',
    'system.tgen.avgWriteLatency',
    'system.tgen.numPackets'
]

filter_records = mem_ctrls + membus + tgen

dump_begin = '---------- Begin Simulation Statistics'
dump_end = '---------- End Simulation Statistics'

# given a stats dump, and a list of filtered record name
# return a list of results
def filter_dump_status(stats_lines, filter_records):
    result = {}
    for each_line in stats_lines:
        line_values = [ x.strip() for x in each_line.split() ]
        #print(line_values)
        if (not line_values):
            continue
        if (line_values[0] in filter_records):
            result[line_values[0]] = line_values[1]
    return result

# given a file yield the stats dump one by one
def grab_dumps(all_file_lines):
    dump_line_start = 0
    dump_line_end = 0
    # scan all lines and find a group of lines for one dump
    for line_no in range(len(all_file_lines)):
        cur_line = all_file_lines[line_no]
        # test for begin
        if (cur_line.startswith(dump_begin)):
            # flag a start point of a new dump stats
            dump_line_start = line_no
        if (cur_line.startswith(dump_end)):
            # flag the end point of the current dump stats
            dump_line_end = line_no
            if (dump_line_start >= dump_line_end):
                raise Exception("Error: start should be less than end")
            # yield the slice of the
            yield all_file_lines[dump_line_start: (dump_line_end + 1)]

def parse_stats_file(file_name, filter):
    ''' Open the file and process with filter
    '''
    all_filtered_dumps = []
    with open(file_name, 'r') as stats_file:
        all_lines = stats_file.readlines()
        for each_dump in grab_dumps(all_lines):
            filtered_dict = filter_dump_status(each_dump, filter)
            #print(filtered_dict)
            all_filtered_dumps.append(filtered_dict)
    return all_filtered_dumps

# given filtered result (a dict) and the filter (a list of keys)
# print out the filtered result
#  key1    value1 value2 value3
#  key2    value1 value2 value3
def print_filtered_dumps(filtered_dumps, filter):
    all_dumps = ''
    all_dumps_without_key = ''
    for key in filter:
        key_values = ''
        # search the key in all dumps one by one
        for idx in range(len(filtered_dumps)):
            cur_dump = filtered_dumps[idx]
            if key in cur_dump:
                # found the key in the dump, grab value
                key_values = key_values  + cur_dump[key] + '\t'
            else:
                # key not found in the dump, use '-' to mark unavailable
                key_values = key_values + '-\t'
        # remove the last \t and add \n to break the line for each key_values
        key_values = key_values.strip() + '\n'
        all_dumps = all_dumps + key + '\t' + key_values
        all_dumps_without_key = all_dumps_without_key + key_values

    print(all_dumps)
    print(all_dumps_without_key)

if __name__ == '__main__':

    if (len(sys.argv) == 1):
        usage()
        exit(-1)

    file_name = sys.argv[1]
    if (not os.path.isfile(file_name)):
        print("Error: %s is not a file" % file_name)
        usage()
        exit(-1)

    all_filtered_dumps = parse_stats_file(file_name, filter_records)
    print_filtered_dumps(all_filtered_dumps, filter_records)
