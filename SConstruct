import os, sys

def createBuildDir():
    targets = """common dynrun test reader  printer pvfregs"""
    if not os.path.exists("build"):
        os.mkdir("build")
    for t in targets.split():
        if not os.path.exists("build/%s" % t):
            print "Creating build/%s" % t
            os.mkdir("build/%s" % t)

def banner(string, color="\033[32m"):
    preString = "=" * 15
    print "%s%s" % (color, preString),
    # not safe for Python < 2.7
    #print "{:^30}".format(string),
    print string,
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


def udisMissing():
    print "\033[31;1mCould not find the udcli binary. Do you have libudis86 installed?\033[0m"
    print "    Please install libudis86 (https://github.com/vmt/udis86) and either make sure"
    print "    its 'udcli' binary is in your path or set the UDISINC and UDISLIB parameters"
    print "    in your scons call to point to the respective directories."
    sys.exit(1)


def udisPath(env):
    import os
    banner("UDIS86 Detection", "\033[36m")

    if ARGUMENTS.get('UDISINC') != None and ARGUMENTS.get('UDISLIB') != None:
        print "Trying include path %s..." % ARGUMENTS.get('UDISINC')
        incpath = ARGUMENTS.get('UDISINC')
        print "Trying lib path %s..." % ARGUMENTS.get('UDISLIB')
        libpath = ARGUMENTS.get('UDISLIB')

    else:
        print "Trying to find udis86 through udcli binary..."
        output = os.popen("which udcli")
        res = output.readlines()
        if len(res) == 0:
            udisMissing()

        path   = os.path.dirname(res[0].strip())
        libpath = path + "/../lib"
        incpath = path + "/../include"

    if os.path.exists(libpath + "/libudis86.a"):
        libp = os.path.realpath(libpath)
        print libp
        env.Append(LIBPATH=[libp])
        env.Append(LIBS=["udis86"])
    else:
        udisMissing()

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
                  CPPFLAGS = ["-isystem", "/home/doebel/local/include", "-g"],
                  LIBS = ["analyzer", "elf"],
                  LIBPATH = ["#/build/common"],
                 )

if ARGUMENTS.get('CLANG') == '1':
    env.Replace(CXX=["clang++"])
    env.Append(CXXFLAGS=["-Weverything", "-Wno-weak-vtables", "-Wno-global-constructors",
                         "-Wno-exit-time-destructors"])

if ARGUMENTS.get('VERBOSE') != '1':
    env['ARCOMSTR']     = "\033[35m[AR    ]\033[0m $TARGET"
    env['CCCOMSTR']     = "\033[35m[CC    ]\033[0m $TARGET"
    env['CXXCOMSTR']    = "\033[35m[CXX   ]\033[0m $TARGET"
    env['LINKCOMSTR']   = "\033[35m[LD    ]\033[0m $TARGET"
    env['RANLIBCOMSTR'] = "\033[35m[RANLIB]\033[0m $TARGET"

if ARGUMENTS.get('CXX') is not None:
    print "Setting CXX:", ARGUMENTS.get("CXX")
    env['CXX'] = ARGUMENTS.get('CXX')

wvtestrun = ""
if not env.GetOption("clean"):
    udisPath(env)
    env = initChecks(env)
    wvtestrun = detect_wv()

banner("Compilation")
SConscript("common/SConscript",  variant_dir="#/build/common",   exports='env')
SConscript("reader/SConscript",  variant_dir="#/build/reader",   exports='env')
SConscript("printer/SConscript", variant_dir="#/build/printer",  exports='env')
SConscript("pvfregs/SConscript", variant_dir="#/build/pvfregs",  exports='env')
SConscript("unroll/SConscript",  variant_dir="#/build/unroll",   exports='env')
SConscript("testing/SConscript", variant_dir="build/test",       exports='env')
SConscript("dynrun/SConscript",  variant_dir="#/build/dynrun",   exports='env')
SConscript("failTrace/SConscript", variant_dir="#/build/failTrace", exports='env')

# make a test run after compilation
env.Append(ENV = {"TERM" : os.environ["TERM"]})

env.testcmd = env.Command("build_always", "", "%s build/test/cfgtest" % wvtestrun)
env.Append(ENV = {"BASEDIR" : os.environ["PWD"]})
env.integrationTests = env.Command("build_always2", "", "testing/scripts/runTests")

env.Depends(env.testcmd, env.test)
env.Depends(env.integrationTests, env.testcmd)
env.Depends(env.integrationTests, env.printer)
env.Depends(env.integrationTests, env.reader)
env.Depends(env.integrationTests, env.pvf)
env.Depends(env.integrationTests, env.unroll)
env.Depends(env.integrationTests, env.dynrun)
