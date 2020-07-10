#!/bin/bash

function Usage() {
 echo "Usage:"
 echo "$0 <trace-file> [recreate trace option]"
 echo "<trace-file>: this must be a qemu trace file that contains"
 echo "     only physical addresses. Format definition in"
 echo "     PhysicalTraceRecord.py"

 echo "option:"
 echo "  0, no need to create new gem5 trace from qemu trace"
 echo "  1, create new gem5 trace from qemu (with bzip2 qemu trace)"
 echo "  2, create new gem5 trace from qemu (with txt qemu trace)"

}

function dir_assert() {
  local dir="$1"
  if [ ! -d "$dir" ];then
    echo "Error: $dir is not a directory or does not exists"
    exit 1
  fi
}

function file_assert() {
  local file="$1"
  if [ ! -f $file ] ; then
	  echo "Error: $file is not a file or does not exist"
	  exit 1
  fi
}

function exist_assert() {
  local file="$1"
  if [ ! -e $file ] ; then
	  echo "Error: $file does not exist"
	  exit 1
  fi
}

function run_cmd() {
  local cmd="$1"
  local pipe_out="$2"
  local has_zip="$3"
  local func="run_cmd"

  if [ "$pipe_out" == '' ];then
   #echo "no pipe out given"
   echo "$cmd"
   eval $cmd
  elif [ "$has_zip" == "gzip" ] ; then
   echo "$func: gzip the console output:"
   echo "$cmd 2>&1 | gzip > $pipe_out"
   eval $cmd 2>&1 | gzip > "$pipe_out"
  elif [ "$has_zip" == "bz2" ] ; then
   echo "$func: bzip2 the console output:"
   echo "$cmd 2>&1 | bzip2 > $pipe_out"
   eval $cmd 2>&1 | bzip2 > "$pipe_out"
  else
   echo "$func: console output as plain txt file:"
   echo "$cmd > $pipe_out 2>&1"
   eval $cmd > $pipe_out 2>&1
  fi
}

###############################################
# post_run
#
# process the output data after gem5 execution
##############################################

function post_run {

    # a subdirectory must be given to store the output stats
    local subdir="$1"

    local has_zip="$2"

    local func="post_run"

    echo "$func: got parameters: $subdir, $has_zip"

    local backup_dir="$log_backup_dir/$subdir"
    local grep_str="======================="

    run_cmd "python $gem5_stats_filter $out_stats" "$out_stats_filt"
    if [ "$has_zip" == "gzip" ] ; then
      echo "$func: gzip output console sink for grep"
      run_cmd "cat $out_console_sink | gzip -d | tee >( tail -100 > $out_console_tail) | grep -a200 '$grep_str'" "$out_console_grep"
    elif [ "$has_zip" == "bz2" ] ; then
      echo "$func: bz2 output console sink for grep"
      run_cmd "bzip2 -k -d -c $out_console_sink | grep -a200 '$grep_str'" "$out_console_grep"
    elif [ "$has_zip" == "" ] ; then
      echo "$func: plain txt console sink for grep"
      run_cmd "grep -a200 '$grep_str' $out_console_sink" "$out_console_grep"
    else
      echo "$func: unknown second (has_zip) option: $has_zip"
      exit 1
    fi

    # check log back dir, warning if exist
    if [ -d $backup_dir ]; then
	    echo "WARNING: back dir $backup_dir already exist, will be overwritten"
    else
	    echo "creating log back dir: $backup_dir"
	    run_cmd "mkdir -p $backup_dir"
    fi

    dir_assert $backup_dir

    for item in $backup_file_list; do
      exist_assert $item
      run_cmd "cp -a $item $backup_dir/"
    done

    # check the console output grep result,
    # if no three lines of iteration pattern '======'
    # probably an error, stop here
    local lines=$(eval "grep '$grep_str' $out_console_grep" | wc -l)
    if [ "$lines" -lt "3" ];then
        echo "$out_console_grep has $lines (less than 3) iterations"
        echo "Error occured during gem5 execution, please check"
        exit 1
    else
	echo "Gem5 succeed ($lines iter lines), now collecting data and backup to $backup_dir"
    fi 


}

function run_notag {

    local init_trace=$1

    local cmd="$gem5_bin --debug-flags=$debug_flags $gem5_config $gem5_common_options $gem5_qemu_trace"

    local pipe_out="$out_console_sink"
    
    local func="run_notag"

    echo "----- $func begin ---------"

    # add reuse-trace and qemu-trace format option according to $1
    if [ "$init_trace" == "1" ];then
      echo "$func: will create new trace from bzip2 QEMU trace file"
    elif [ "$init_trace" == "2" ];then
      echo "$func: will create new trace from txt QEMU trace file"
      cmd+=" --qemu-trace-is-txt"
    elif [ "$init_trace" == "0" ];then
      echo "$func: will reuse qemu trace"
      cmd="$cmd --reuse-trace"
    else
      echo "$func: must give 0, 1 or 2 as parameter"
    fi


    run_cmd "$cmd" "$pipe_out" "gzip"

    local log_sub_dir="notag"
    post_run "$log_sub_dir" "gzip"

    echo "-----run_notag done ---------"
}

function run_tag_excl {

    local init_trace=$1

    local cmd="$gem5_bin --debug-flags=$debug_flags $gem5_config $gem5_common_options $gem5_qemu_trace"
    cmd="$cmd --enable-shadow-tags"

    local pipe_out="$out_console_sink"

    local func="run_tag_excl"

    echo "----- $func begin ---------"

    # add reuse-trace and qemu-trace format option according to $1
    if [ "$init_trace" == "1" ];then
      echo "$func: will create new trace from bzip2 QEMU trace file"
    elif [ "$init_trace" == "2" ];then
      echo "$func: will create new trace from txt QEMU trace file"
      cmd+=" --qemu-trace-is-txt"
    elif [ "$init_trace" == "0" ];then
      echo "$func: will reuse qemu trace"
      cmd="$cmd --reuse-trace"
    else
      echo "$func: must give 0, 1 or 2 as parameter"
    fi

    #run_cmd "$cmd" "$pipe_out" "gzip"

    local log_sub_dir="tag_excl"
    post_run "$log_sub_dir" "gzip"
    #exit 1
}


function run_tag_incl {

    local init_trace=$1

    local cmd="$gem5_bin --debug-flags=$debug_flags $gem5_config $gem5_common_options $gem5_qemu_trace"
    cmd="$cmd --enable-shadow-tags --tagcache-inclusive"

    local pipe_out="$out_console_sink"

    local func="run_tag_incl"

    echo "----- $func begin ---------"

    # add reuse-trace and qemu-trace format option according to $1
    if [ "$init_trace" == "1" ];then
      echo "$func: will create new trace from bzip2 QEMU trace file"
    elif [ "$init_trace" == "2" ];then
      echo "$func: will create new trace from txt QEMU trace file"
      cmd+=" --qemu-trace-is-txt"
    elif [ "$init_trace" == "0" ];then
      echo "$func: will reuse qemu trace"
      cmd="$cmd --reuse-trace"
    else
      echo "$func: must give 0, 1 or 2 as parameter"
    fi

    run_cmd "$cmd" "$pipe_out" "gzip"

    local log_sub_dir="tag_incl"
    post_run "$log_sub_dir" "gzip"

}

###########################################
# Main entry of the program    ############
###########################################
# Get QEMU trace file as input
# set a default value for trace file
qemu_trace_file="qemu_trace/test.txt.bz2"
create_new_trace="0"


if [ "$1" == "" ]; then
 echo "Error: must give an qemu trace file name"
 Usage
 exit 1
else
 qemu_trace_file="$1"
 file_assert "$1"
fi

if [ "$2" == "1" ]; then
 create_new_trace="1"
elif [ "$2" == "0" ];then
 create_new_trace="0"
elif [ "$2" == "" ]; then
 create_new_trace="0"
elif [ "$2" == "2" ]; then
 create_new_trace="2"
else
 echo "Error: second paramter not recognized."
 Usage
 exit 1
fi
 

#######################################
# Configurable variables in this script
#
# Gem5 setup
gem5_root="$HOME/lab/cheri/beri/tag-sim/gem5"
gem5_bin="./build/X86/gem5.opt"
gem5_config="configs/tupipa/replay_qemu_mem.py"

gem5_qemu_trace="--qemu-trace=$qemu_trace_file"
# will create a log back up dir for the trace file
log_backup_dir="$qemu_trace_file-m5out"

gem5_trace_file="$log_backup_dir/gem5.trc.gz"

gem5_common_options="--mem-size=4096MB --req-size=1"
gem5_common_options+=" --gem5-trace=$gem5_trace_file"

debug_flags="Cache,TagController,CoherentXBar,DRAM,MemoryAccess,Checkpoint,TrafficGen"

# Output handling
out_console_sink="m5out/console.txt.gz"
out_stats="m5out/stats.txt"
out_cfg="m5out/traffic-gen.cfg"
out_config_ini="m5out/config.ini"
out_config_json="m5out/config.json"

out_gem5_trace_total="$gem5_trace_file-total.txt"

gem5_stats_filter="qemu_trace/filter_stats.py"

out_stats_filt="m5out/stats.txt.filt"
out_console_grep="m5out/console-grep.md"
out_console_tail="m5out/console-tail.md"

backup_file_list="$out_stats $out_stats_filt $out_console_grep $out_console_tail $out_cfg"
backup_file_list="$backup_file_list $out_config_ini $out_config_json"

# backup_file_list_common="$out_gem5_trace_total"
# backup_file_list_common+=" $gem5_trace_file"

# Done Configurable zones in this script
##############################################

# check gem5 root dir

dir_assert "$gem5_root"
cd $gem5_root

# check log backup dir
# create if not exist
if [ ! -d "$log_backup_dir" ]; then
  # if log backup dir does not exist, 
  # that means the gem5 trace also not created, so check we have
  # 'create_new_trace' option on
  if [ "$create_new_trace" == "0" ];then
     echo "Error: trace directory $log_backup_dir does not exist"
     echo "So the create new trace option must be on (1)"
     Usage
     exit 1
  fi
  echo "creating log back dir: $log_backup_dir"
  run_cmd "mkdir -p $log_backup_dir"
fi

run_tag_excl "$create_new_trace"
run_tag_incl 0
run_notag 0

# test post_run
#post_run "tag_excl"


