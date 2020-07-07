#!/bin/bash

#######################################
# Configurable variables in this script
#
# Gem5 setup
gem5_root="$HOME/lab/cheri/beri/tag-sim/gem5"
gem5_bin="./build/X86/gem5.opt"
gem5_config="configs/tupipa/replay_qemu_mem.py"
gem5_qemu_trace="--qemu-trace=qemu_trace/test.txt.bz2"
gem5_common_options="--mem-size=4096MB --req-size=1 $gem5_qemu_trace"

debug_flags="Cache,TagController,CoherentXBar,DRAM,MemoryAccess,Checkpoint,TrafficGen"

# Output handling
out_console_sink="m5out/console.txt"
out_stats="m5out/stats.txt"
out_cfg="m5out/lat_mem_rd.cfg"
out_config_ini="m5out/config.ini"
out_config_json="m5out/config.json"
out_total="m5out/lat_mem_rd.trc.gz-total.txt"

gem5_stats_filter="qemu_trace/filter_stats.py"

out_stats_filt="m5out/stats.txt.filt"
out_console_grep="m5out/console-grep.md"

backup_file_list="$out_stats $out_stats_filt $out_console_grep $out_cfg"
backup_file_list="$backup_file_list $out_config_ini $out_config_json"
backup_file_list="$backup_file_list $out_total"
log_backup_dir="test/"

# Done Configurable zones in this script
##############################################

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
   $cmd
  else
   echo "$cmd > $pipe_out 2>&1"
   $cmd > "$pipe_out"  2>&1
  fi
}

###############################################
# post_run
#
# process the output data after gem5 execution
##############################################

function post_run {

    run_cmd "python $gem5_stats_filter $out_stats" "$out_stats_filt"
    run_cmd "grep -a200 '=====================' $out_console_sink" "$out_console_grep"

    # check log back dir, warning if exist
    if [ -d $log_backup_dir ]; then
	    echo "WARNING: back dir $log_backup_dir already exist, will be overwritten"
    else
	    echo "creating log back dir: $log_backup_dir"
	    run_cmd "mkdir -p $log_backup_dir"
    fi

    #dir_assert $log_backup_dir

    for item in $backup_file_list; do
      exist_assert $item
      run_cmd "cp -a $item $log_backup_dir"
    done

}

function run_notag {

    init_trace=$1

    cmd="$gem5_bin --debug-flags=$debug_flags $gem5_config $gem5_common_options"

    pipe_out="$out_console_sink"

    # add reuse-trace option according to $1
    if [ "$init_trace" == "1" ];then
      echo "run_tag_incl: will create new trace from QEMU input"

    elif [ "$init_trace" == "0" ];then

      echo "run_tag_incl: reuse trace"
      cmd="$cmd --reuse-trace"

    fi
    run_cmd "$cmd" "$pipe_out"

    post_run

}

function run_tag_excl {

    init_trace=$1

    cmd="$gem5_bin --debug-flags=$debug_flags $gem5_config $gem5_common_options"
    cmd="$cmd --enable-shadow-tags"

    pipe_out="$out_console_sink"

    # add reuse-trace option according to $1
    if [ "$init_trace" == "1" ];then
      echo "run_tag_excl: will create new trace from QEMU input"

    elif [ "$init_trace" == "0" ];then

      echo "run_tag_excl: reuse trace"
      cmd="$cmd --reuse-trace"
    fi

    run_cmd "$cmd" "$pipe_out"

    post_run

}


function run_tag_incl {

    init_trace=$1

    cmd="$gem5_bin --debug-flags=$debug_flags $gem5_config $gem5_common_options"
    cmd="$cmd --enable-shadow-tags --tagcache-inclusive"

    pipe_out="$out_console_sink"

    # add reuse-trace option according to $1
    if [ "$init_trace" == "1" ];then
      echo "run_tag_incl: will create new trace from QEMU input"

    elif [ "$init_trace" == "0" ];then

      echo "run_tag_incl: reuse trace"
      cmd="$cmd --reuse-trace"

    fi

    run_cmd "$cmd" "$pipe_out"

    post_run

}

# check gem5 root dir

cd $gem5_root
run_tag_excl 0
run_tag_incl
run_notag


