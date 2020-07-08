#!/bin/bash

function Usage() {
 echo "Usage:"
 echo "$0 <trace-file> [recreate trace option]"
 echo "<trace-file>: this must be a qemu trace file that contains"
 echo "     only physical addresses. Format definition in"
 echo "     PhysicalTraceRecord.py"

 echo "option:"
 echo "  0, no need to create new gem5 trace from qemu trace"
 echo "  1, create new gem5 trace from qemu"

}

function dir_assert() {
  dir="$1"
  if [ ! -d "$dir" ];then
    echo "Error: $dir is not a directory or does not exists"
    exit 1
  fi
}

function file_assert() {
  file="$1"
  if [ ! -f $file ] ; then
	  echo "Error: $file is not a file or does not exist"
	  exit 1
  fi
}

function exist_assert() {
  file="$1"
  if [ ! -e $file ] ; then
	  echo "Error: $file does not exist"
	  exit 1
  fi
}

function run_cmd() {
  cmd="$1"
  pipe_out="$2"
  if [ "$pipe_out" == '' ];then
   echo "no pipe out given"
   echo "$cmd"
   eval $cmd
  else
   echo "$cmd > $pipe_out 2>&1"
   eval $cmd > "$pipe_out"  2>&1
  fi
}

###############################################
# post_run
#
# process the output data after gem5 execution
##############################################

function post_run {

    # a subdirectory must be given to store the output stats
    subdir="$1"

    backup_dir="$log_backup_dir/$subdir"

    run_cmd "python $gem5_stats_filter $out_stats" "$out_stats_filt"
    run_cmd "grep -a200 '=====================' $out_console_sink" "$out_console_grep"

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

}


# function backup_common {

#     backup_dir="$1"

#     # check log back dir, warning if exist
#     if [ -d $backup_dir ]; then
# 	    echo "copy commonly shared files into $backup_dir"
#     else
# 	    echo "Error: back dir: $backup_dir does not exist"
# 	    exit 1
#     fi

#     dir_assert $backup_dir

#     for item in $backup_file_list_common; do
#       exist_assert $item
#       run_cmd "cp -a $item $backup_dir/"
#     done

  
# }

function run_notag {

    init_trace=$1

    cmd="$gem5_bin --debug-flags=$debug_flags $gem5_config $gem5_common_options $gem5_qemu_trace"

    pipe_out="$out_console_sink"

    # add reuse-trace option according to $1
    if [ "$init_trace" == "1" ];then
      echo "run_tag_incl: will create new trace from QEMU input"

    elif [ "$init_trace" == "0" ];then

      echo "run_tag_incl: reuse trace"
      cmd="$cmd --reuse-trace"

    else
      echo "run_tag_excl: must give 0 or 1 as parameter"
    fi

    run_cmd "$cmd" "$pipe_out"

    log_sub_dir="notag"
    post_run "$log_sub_dir"

}

function run_tag_excl {

    init_trace=$1

    cmd="$gem5_bin --debug-flags=$debug_flags $gem5_config $gem5_common_options $gem5_qemu_trace"
    cmd="$cmd --enable-shadow-tags"

    pipe_out="$out_console_sink"

    # add reuse-trace option according to $1
    if [ "$init_trace" == "1" ];then
      echo "run_tag_excl: will create new trace from QEMU input"

    elif [ "$init_trace" == "0" ];then

      echo "run_tag_excl: reuse trace"
      cmd="$cmd --reuse-trace"

    else
      echo "run_tag_excl: must give 0 or 1 as parameter"
    fi

    run_cmd "$cmd" "$pipe_out"

    log_sub_dir="tag_excl"
    post_run "$log_sub_dir"
}


function run_tag_incl {

    init_trace=$1

    cmd="$gem5_bin --debug-flags=$debug_flags $gem5_config $gem5_common_options $gem5_qemu_trace"
    cmd="$cmd --enable-shadow-tags --tagcache-inclusive"

    pipe_out="$out_console_sink"

    # add reuse-trace option according to $1
    if [ "$init_trace" == "1" ];then
      echo "run_tag_incl: will create new trace from QEMU input"

    elif [ "$init_trace" == "0" ];then

      echo "run_tag_incl: reuse trace"
      cmd="$cmd --reuse-trace"

    else
      echo "run_tag_incl: must give 0 or 1 as parameter"
    fi

    run_cmd "$cmd" "$pipe_out"

    log_sub_dir="tag_incl"
    post_run "$log_sub_dir"

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
out_console_sink="m5out/console.txt"
out_stats="m5out/stats.txt"
out_cfg="m5out/traffic-gen.cfg"
out_config_ini="m5out/config.ini"
out_config_json="m5out/config.json"

out_gem5_trace_total="$gem5_trace_file-total.txt"

gem5_stats_filter="qemu_trace/filter_stats.py"

out_stats_filt="m5out/stats.txt.filt"
out_console_grep="m5out/console-grep.md"

backup_file_list="$out_stats $out_stats_filt $out_console_grep $out_cfg"
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
  if [ "$create_new_trace" != "1" ];then
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
# backup_common "$log_backup_dir"

