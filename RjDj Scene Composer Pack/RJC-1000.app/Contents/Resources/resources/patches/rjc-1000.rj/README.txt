rjc1000 dynamic module patching


Open _main.pd

It load the following setup description:

decor-m1-m1 TEST1 module-m1-m1
decor-m2-m1 TEST2 module-m2-m1
decor-m2-m2 TEST3 module-m2-m2
decor-s1m1-s1m1 TEST4 module-s1m1-s1m1
decor-s1m1-s2m1 TEST5 module-s1m1-s2m1

Here "decor-*" are the decorators responsible for handling inter-module
communication paths. The "m1" or "s2" indicate the number of message or signal
inlets a module has. Example modules are included. 

Each "decor-*" abstractions requires at least two arguments: A *unique* tag,
like "TEST3" and the name of the module to manage. Arguments after that are
ignored. 

preliminary Specs 
------------------

a decorator should:
- send messages to specific inlets of a decorated module
    [x] Implementation: send to global receiver RJC1000-M-IN, then route by $1 and inlet number 
- send messages to specific inlets of all modules
    [ ] Implementation: send to global receiver RJC1000-M-IN routed by "ALL" and inlet number?
- receive messages from specific inlets of a module
    [ ] Implementation: receive from global receiver, then route by decorator's $1 and inlet
    [s RJC1000-M-OUT] in decorator, plus [list prepend $1 <inlet>]
- receive messages from specific inlets of all modules (not needed?)
- send audio to specific inlets of a specific module
    [ ] Implementation: [catch~ $1-S-IN/$1-S-IN-0/$1-S-IN-1] in decorator
- send audio to all modules
    [ ] Implementation: [r~ RJC1000-S-IN] in decorator
- receive audio from a specific inlet of a specific module
    [ ] Implementation: [throw~ $1-OUT/$1-OUT-0/$1-OUT-1] in decorator
- receive audio from a all modules
    [ ] Implementation: [throw~ RJC1000-S-OUT] in decorator
- send control messages to specific and all decorators (e.g. switch on/off)
