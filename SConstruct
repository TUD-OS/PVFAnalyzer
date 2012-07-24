targets = """common test cfg"""

import os
if not os.path.exists("build"):
    os.mkdir("build")
for t in targets.split():
    if not os.path.exists("build/%s" % t):
        print "Creating build/%s" % t
        os.mkdir("build/%s" % t)

def udis_path(env):
    import os
    print "Detecting UDIS86 path...",
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
udis_path(env)

SConscript("common/SConscript",   variant_dir="build/common", exports='env')
SConscript("analyzer/SConscript", variant_dir="build/cfg",    exports='env')
SConscript("testing/SConscript",  variant_dir="build/test",   exports='env')

env.Command("build_always", "", "build/test/cfgtest")
