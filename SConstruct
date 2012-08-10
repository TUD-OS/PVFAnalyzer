import os, sys

def createBuildDir():
    targets = """common test cfg_reader  cfg_printer"""
    if not os.path.exists("build"):
        os.mkdir("build")
    for t in targets.split():
        if not os.path.exists("build/%s" % t):
            print "Creating build/%s" % t
            os.mkdir("build/%s" % t)

def banner(string, color="\033[32m"):
    preString = "=" * 15
    print "%s%s" % (color, preString),
    print "{:^30}".format(string),
    print "%s\033[0m" % (preString)


def initChecks(env):
    banner("Configuration", "\033[33m")

    unique_headers = """
boost/archive/binary_iarchive.hpp
boost/archive/binary_oarchive.hpp
boost/foreach.hpp
boost/graph/adjacency_list.hpp
boost/graph/adj_list_serialize.hpp
boost/graph/graph_concepts.hpp
boost/graph/graphviz.hpp
boost/serialization/export.hpp
boost/serialization/serialization.hpp
boost/tuple/tuple.hpp
    """

    conf = Configure(env)
    if not conf.CheckCC(): sys.exit(1)
    if not conf.CheckCXX(): sys.exit(1)
    for f in unique_headers.split():
        if not conf.CheckCXXHeader(f): sys.exit(1)

    # Apparently, a SCons bug breaks dependencies if we use
    # the checks below :(
    #conf.CheckLibWithHeader("udis86", "udis86.h", "C++")
    #conf.CheckLib("boost_serialization", language="C++")

    return conf.Finish()

def udisPath(env):
    import os
    banner("UDIS86 Detection", "\033[36m")

    output = os.popen("which udcli")
    path   = os.path.dirname(output.readlines()[0].strip())
    libpath = path + "/../lib"
    incpath = path + "/../include"
    #print path
    if os.path.exists(libpath + "/libudis86.a"):
        libp = os.path.realpath(libpath)
        print libp
        env.Append(LIBPATH=[libp])
        env.Append(LIBS=["udis86"])
    else:
        print "\033[31;1mlibudis86 not found.\033[0m"
        sys.exit(1)

    if os.path.exists(incpath + "/udis86.h"):
        incp = os.path.realpath(incpath)
#        env.Append(CPPPATH=[incp])
        env.Append(CPPFLAGS=["-isystem", incp])
    else:
        print "\033[31;1mudis86.h not found.\033[0m"
        sys.exit(1)

def detect_wv():
    banner("Detecting wvtestrun", "\033[36m")
    ret = os.popen("which wvtestrun").read().strip()
    if ret != "":
        print ret
    else:
        print "<not found>"
    return ret

createBuildDir()

env = Environment(CPPPATH = ["#/common"],
                  CCFLAGS = ["-std=c++0x","-Weffc++", "-Wall", "-g"],
                  CPPFLAGS = ["-isystem", "/home/doebel/local/include"],
                  LIBS = ["analyzer"],
                  LIBPATH = ["#/build/common"],
                 )

if ARGUMENTS.get('VERBOSE') != '1':
    env['ARCOMSTR']     = "\033[35m[AR    ]\033[0m $TARGET"
    env['CCCOMSTR']     = "\033[35m[CC    ]\033[0m $TARGET"
    env['CXXCOMSTR']    = "\033[35m[CXX   ]\033[0m $TARGET"
    env['LINKCOMSTR']   = "\033[35m[LD    ]\033[0m $TARGET"
    env['RANLIBCOMSTR'] = "\033[35m[RANLIB]\033[0m $TARGET"

wvtestrun = ""
if not env.GetOption("clean"):
    udisPath(env)
    env = initChecks(env)
    wvtestrun = detect_wv()

banner("Compilation")
SConscript("common/SConscript",      variant_dir="#/build/common",     exports='env')
SConscript("cfg_reader/SConscript",  variant_dir="#/build/cfg_reader", exports='env')
SConscript("cfg_printer/SConscript", variant_dir="#/build/cfg_printer",exports='env')
SConscript("testing/SConscript",     variant_dir="build/test",       exports='env')

# make a test run after compilation
env.Append(ENV = {"TERM" : os.environ["TERM"]})
env.testcmd = env.Command("build_always", "", "%s build/test/cfgtest" % wvtestrun)

env.Depends(env.testcmd, env.test)
