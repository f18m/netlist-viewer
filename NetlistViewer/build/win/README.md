# Building NetListViewer on Windows

Please download and compile with latest VisualStudio both 
1. wxWidgets >= 3.0.0
1. Boost >= 1.58

Then define the following 2 environment variables:
1. WXWIN        to point to the base root of your wxWidgets directory
1. BOOST_DIR    to point to the base root of your Boost directory
 
For more info on how to do it, please look on Google, e.g.: https://superuser.com/questions/949560/how-do-i-set-system-environment-variables-in-windows-10

Finally open the netlist_viewer_vs2022.sln project with VisualStudio and you should be able to build it.



