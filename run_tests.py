# run using:
#   python3 run_tests.py
import subprocess
import os
import glob
from collections import namedtuple

TEST_DIR = "tests"
TMP_DIR = "test_output"

SubprocessResult = namedtuple('SubprocessResult', ['returncode', 'stdout', 'stderr'])
TIMEOUT_SIGNAL = -1

def run(*args, **kwargs):
    try:
        p = subprocess.run(*args, preexec_fn=os.setsid, **kwargs)
        code = p.returncode
        output = p.stdout
        error = p.stderr
    except subprocess.TimeoutExpired as e:
        timeout_str = "Timeout ({}s) expired!".format(e.timeout)
        code = TIMEOUT_SIGNAL
        output = timeout_str
        error = timeout_str
    return SubprocessResult(returncode=code, stdout=output, stderr=error)

files = glob.glob(TEST_DIR+"/*.ll") + glob.glob(TEST_DIR+"/*.c")

run(["mkdir", "-p", TMP_DIR], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

# might have to run source ~/.bash_profile

# make the helper file
HELPER_FILE = "./extras.c"
HELPER_OUT = "./extras.o"
CLANG_EXEC = "/usr/local/opt/llvm/bin/clang"
OPT_EXEC = "/usr/local/opt/llvm/bin/opt"
run([CLANG_EXEC, "-c", HELPER_FILE], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

# make binary
make_bin = run(["make"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, cwd="build")
if make_bin.returncode != 0:
    print("FAILED! Unable to build")
    print(make_bin.stdout.decode('utf-8'))
else:
    print("Build succeeded")

for file in files:
    basename = os.path.basename(file)
    base = os.path.splitext(basename)[0]
    is_ll = os.path.splitext(basename)[1] == ".ll"
    print(basename)
    test_dir = os.path.join(TMP_DIR, base)
    run(["mkdir", "-p", test_dir], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    ll_name = os.path.join(test_dir, base+".ll")
    if is_ll:
        run(["cp", file, ll_name], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    else:
        make_ll = run([CLANG_EXEC, "-emit-llvm", "-S", file, "-o", ll_name], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if make_ll.returncode != 0:
            print("FAILED! Unable to make llvm")
            print(make_ll.stdout.decode('utf-8'))

    run([OPT_EXEC, "-instnamer", "-S", ll_name, "-o", ll_name], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    prof_ll = os.path.join(test_dir, base+"_prof.ll")
    run_opt = run([OPT_EXEC, "-load", "build/skeleton/libSkeletonPass.so", "-skeleton", "-S", ll_name, "-o", prof_ll], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if run_opt.returncode != 0:
        print("FAILED! Unable to build")
        print(run_opt.stdout.decode('utf-8'))
    else:
        opt_stdout = os.path.join(test_dir, base+"_prof.stdout")
        f = open(opt_stdout, "w+")
        f.write(run_opt.stdout.decode('utf-8'))
        f.close()

    # create prof cfgs
    make_prof_cfg = run([OPT_EXEC, "-dot-cfg-only", "-S", prof_ll], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if make_prof_cfg.returncode != 0:
        print("FAILED! Unable to make prof cfg")
        print(make_prof_cfg.stdout.decode('utf-8'))
    for cfg_file in glob.glob("./.*.dot"):
        cfg_func_name = os.path.basename(cfg_file)
        cfg_func_base = os.path.splitext(cfg_func_name)[0][1:]
        dot_file = os.path.join(test_dir, cfg_func_base+"_prof.png")
        make_dot = run(["dot", "-Tpng", cfg_file, "-o", dot_file], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if make_dot.returncode != 0:
            print("FAILED! Unable to make dot")
            print(make_dot.stdout.decode('utf-8'))
        run(["rm", cfg_file], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    # create normal cfgs
    make_normal_cfg = run([OPT_EXEC, "-dot-cfg-only", "-S", ll_name], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if make_normal_cfg.returncode != 0:
        print("FAILED! Unable to make normal cfg")
        print(make_normal_cfg.stdout.decode('utf-8'))
    for cfg_file in glob.glob("./.*.dot"):
        cfg_func_name = os.path.basename(cfg_file)
        cfg_func_base = os.path.splitext(cfg_func_name)[0][1:]
        dot_file = os.path.join(test_dir, cfg_func_base+".png")
        make_dot = run(["dot", "-Tpng", cfg_file, "-o", dot_file], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if make_dot.returncode != 0:
            print("FAILED! Unable to make dot")
            print(make_dot.stdout.decode('utf-8'))
        run(["rm", cfg_file], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)


    normal_out = os.path.join(test_dir, base+"_prof.o")
    make_normal_o = run([CLANG_EXEC, "-c", ll_name, "-o", normal_out], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if make_normal_o.returncode != 0:
        print("FAILED! Unable to make normal o")
        print(make_normal_o.stdout.decode('utf-8'))
    test_bin = os.path.join(test_dir, "test.bin")
    make_test_bin = run([CLANG_EXEC, HELPER_OUT, normal_out, "-o", test_bin], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if make_test_bin.returncode != 0:
        print("FAILED! Unable to make test bin")
        print(make_test_bin.stdout.decode('utf-8'))
    
    run_bin = run(["./"+test_bin], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    normal_stdout = os.path.join(test_dir, "test.stdout")
    f = open(normal_stdout, "w+")
    f.write(run_bin.stdout.decode('utf-8'))
    f.write("\nReturn code: "+str(run_bin.returncode))
    f.close()
    print("Ran normal file")

    prof_out = os.path.join(test_dir, base+"_prof.o")
    make_prof_o = run([CLANG_EXEC, "-c", prof_ll, "-o", prof_out], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if make_prof_o.returncode != 0:
        print("FAILED! Unable to make prof o")
        print(make_prof_o.stdout.decode('utf-8'))
    test_prof_bin = os.path.join(test_dir, "test_prof.bin")
    make_prof_bin = run([CLANG_EXEC, HELPER_OUT, prof_out, "-o", test_prof_bin], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if make_prof_bin.returncode != 0:
        print("FAILED! Unable to make prof bin")
        print(make_prof_bin.stdout.decode('utf-8'))
    
    run_prof_bin = run(["./"+test_prof_bin], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    prof_stdout = os.path.join(test_dir, "test_prof.stdout")
    f = open(prof_stdout, "w+")
    f.write(run_prof_bin.stdout.decode('utf-8'))
    f.write("\nReturn code: "+str(run_prof_bin.returncode))
    f.close()
    print("Ran prof file")

print("Finished running tests")
# run(["rm","-rf", TMP_DIR], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)