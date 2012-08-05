targets = """common test cfg"""

import os
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
cassert
cerrno
climits
cstddef
cstdint
cstdio
cstdlib
cstring
ctype.h
direct.h
errno.h
fstream
getopt.h
iomanip
iostream
libelf.h
signal.h
stdio.h
stdlib.h
string
string.h
sys/wait.h
time.h
udis86.h
unistd.h
vector
    """

    conf = env.Configure()
    conf.CheckCC()
    conf.CheckCXX()
    for f in unique_headers.split():
        conf.CheckCXXHeader(f)
    conf.CheckLibWithHeader("udis86", "udis86.h", "C++")
    conf.CheckLib("boost_serialization", language="C++")

def udisPath(env):
    import os
    banner("UDIS86 Detection", "\033[34m")

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
        env.Append(CPPPATH=[incp])
    else:
        print "\033[31;1mudis86.h not found.\033[0m"
        sys.exit(1)


env = Environment(CPPPATH = ["#/common"],
                  CCFLAGS = ["-std=c++0x","-Weffc++", "-Wall", "-g"],
                  LIBS = ["analyzer"],
                  LIBPATH = ["#/build/common"],
                 )
udisPath(env)
initChecks(env)

banner("Compilation")
SConscript("common/SConscript",   variant_dir="build/common", exports='env')
SConscript("analyzer/SConscript", variant_dir="build/cfg",    exports='env')
SConscript("testing/SConscript",  variant_dir="build/test",   exports='env')

# make a test run after compilation
env.testcmd = env.Command("build_always", "", "build/test/cfgtest")

Depends(env.analyzer, env.analyzerlib)
Depends(env.test    , env.analyzerlib)
Depends(env.testcmd , env.test)

# For some reason, scons -c does not seem to remove the
# library created. So, we need to have a dedicated clean
# rule to erase it.
env.Clean(".", "build/common/libanalyzer.a")
