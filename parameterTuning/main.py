#!/usr/bin/env python3

import subprocess
import os
import tempfile
import signal
import time
import itertools
try:
    import cPickle as pickle
except:
    import pickle

# int == value
def solution_value(solutionFile):
    solutionFile.seek(0)
    return (int(solutionFile.readline().split()[1]))


def compile_with_additional_flags(target, flag_name, flag_value):
    """
    cleans and freshly compiles with new additional parameter
    """
    additional = "ADDITIONAL_CFLAGS=-D{}={}".format(flag_name, flag_value)
    subprocess.call(["make", "clean_tuning"])
    subprocess.call(["make", target, additional])

# [ unsigned ] == value of the solution
def run_test(tuning_parameter, tuning_value, ttl, seeds, test_files, prefix=""):
    ## compile with parameret value
    target = "parameterTuning/star_contractions_test"
    compile_with_additional_flags(target, tuning_parameter, tuning_value)
    ## run test with all seeds
    print("{}: Spawning childs".format(prefix))
    runs = []
    for test_file, seed in itertools.product(test_files, seeds):
        infile = open(test_file)        # leaving these open!!
        tmp = tempfile.NamedTemporaryFile()
        print("{}calling:\t{} -s {} <{}".format(prefix, target, seed, test_file))
        p = subprocess.Popen([target, "-s {}".format(seed)],
                    stdin=infile,
                    stdout=tmp,
                    stderr=subprocess.DEVNULL,
                    preexec_fn=os.setpgrp
                    )
        print("{} pid {}".format(prefix, p.pid))
        runs.append( (p, tmp, test_file) )

    print("{} \t=>waiting children for about {} seconds".format(prefix, ttl))
    time.sleep(ttl)
    for p, _, _ in runs:
        print("{}\tsending SIGTERM to pid {}".format(prefix, p.pid))
        os.killpg(p.pid, signal.SIGTERM)
   
    print("{} Waiting for childs to output their solution".format(prefix))
    time.sleep(30)
    for p, _, _ in runs:
        s = os.waitpid(p.pid, 0)[1]

    # collect solutions
    result = dict()
    collect = [ (t, solution_value(f)) for _, f, t in runs]
    for _, _, t in runs:
        result[t] = sum( [a for z,a in collect if z == t] )

    for _, tmp, _ in runs:
        tmp.close()
    return (result)

if __name__ == '__main__':
    ## parameters to tune (with defaults):
    #CONST_VERT_SIZES = "{1, 2, 2, 2, 3, 4, 4, 7, 4, 1}"
    #CONST_TRIES_AFTER_RESET = 20
    #CONST_STAR_CONTRACTIONS_TIME 23*60
    #CONST_ABOVE_CURRENT_WEIGHT 1.001
    #CONST_ABOVE_BEST_WEIGHT 1.02

    ## parameters of tests
    ttl = 15*60
    seeds = [123562, 118182, 363791, 1524, 182894, 186564, 956847534, 151623, 18635864, 5632]
    #seeds = [42, 1524]

    to_tune = 'CONST_STAR_CONTRACTIONS_TIME'
    tune_vals = [60*a for a in range(3,(ttl // 60)+1)]
    #tune_vals = [60]
    instances = (1, 17, 25, 75, 101, 123, 163, 197)
    #instances = (1, )
    test_files = [os.path.join( os.getcwd(), "data", "instance{:03}.gr".format(num)) for num in instances]

    results = dict()
    for tnum, tune_val in enumerate(tune_vals):
        results[tune_val] = run_test(to_tune, tune_val, ttl, seeds, test_files, prefix=(tnum+1)*"*")

    settings = ""

    with open(os.path.join( os.getcwd(), "parameterTuning", "tuning_data_{}.pkl".format(to_tune)), 'wb') as f:
        pickle.dump(settings, f, protocol=2)
        pickle.dump(results, f, protocol=2)
