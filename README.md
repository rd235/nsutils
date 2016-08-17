#NSUTILS: Linux namespace utilities

Nsutils suite includes a number of utilities to list, add/remove tag, and join namespaces.

INSTALL:

get the source code, from the root of the source tree run:
```
$ autoreconf -if
$ ./configure
$ make
$ sudo make install
```

**nslist**, **nshold** and **nsrelease** restrict their action to one type
of namespaces using one of the following prefixes:
**cgroup**, **ipc**, **mnt**, **net**, **pid**, **user**, **uts**.  
i.e. use **nslist** to list namespaces of any kind, **netnslist** to list
network namespaces only.

The suite includes a set of man pages providing a complete description of the tools and a detailed commented 
list of all the options.

This page gives just some examples of common usage cases to show what
*nsutils* are useful for.

## nslist: list the namespaces

- list the name of all the namespaces (in the entire system if run by root, available to the current user otherwise).
```
$ nslist -ss
cgroup:[4026531835]
ipc:[4026531839]
mnt:[4026531840]
mnt:[4026532879]
net:[4026531969]
net:[4026532508]
net:[4026532883]
pid:[4026531836]
user:[4026531837]
uts:[4026531838]
```

- list all the net namespaces and all the processes running in each namespace:
```
$ netnslist
net:[4026531969]
     PID CMDLINE
   31909 /lib/systemd/systemd --user

net:[4026532508]
     PID CMDLINE
   19395 bash

net:[4026532883]
     PID CMDLINE
    8782 -bash
    8939 netnslist
```

- list all the pids of the processes running in a specific namespace:
```
$ nslist -p net:[4026532883]
8782
8989
```

- count the processes running in two namespaces
```
$ nslist -c net:[4026532508] net:[4026532883]
3
```

- return the pid namespace of a process (as nsid or as ns number)
```
$ pidnslist -ss 8782
pid:[4026531836]
$ pidnslist -ssn 8782
4026531836
```

- show all the namespaces of process matching a regular expression against
  the process name
```
$ netnslist -R "bash"
net:[4026532508]
     PID CMDLINE
   19395 bash

net:[4026532883]
     PID CMDLINE
    8782 -bash
```

- the same above but in a tabular form
```
$ netnslist -T -R "bash"
          NAMESPACE      PID CMDLINE
   net:[4026532508]    19395 bash
   net:[4026532883]     8782 -bash
```

- list the namespaces of a specific user (for root. All the commands above
can be restricted to the processes of a specific user by the `-U` option) 
```
# nslist -U myuser -ss
cgroup:[4026531835]
ipc:[4026531839]
mnt:[4026531840]
mnt:[4026532879]
net:[4026531969]
net:[4026532508]
net:[4026532883]
pid:[4026531836]
user:[4026531837]
uts:[4026531838]
```

- list the all the namespaces where a 'vim' process is running and show
include the owner of the processes in the list:
```
# mntnslist -u -R vim
mnt:[4026531840]
     PID USER      CMDLINE
    9368 renzo     vim -c set textwidth=75 nocindent README.md
    9492 renzo     vim xx
```

## netnsjoin: join another network namespace

**netnsjoin** runs a command in the net namespace of another process.
This command requires root access or at least the CAP_NET_ADMIN capability.
Users with CAP_NET_ADMIN capability are allowed to join all the namespaces
thay have access to but the *real* namespace (i.e. the net namespace of
process 1).

**netnsjoin** does not need the net namespace to be *mounted*, see
mount(2).

```
# ip addr
1: lo: <LOOPBACK> mtu 65536 qdisc noop state DOWN group default qlen 1
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
# netnslist -ss $$
net:[4026532883]
# netnsjoin 1 bash
Joining net namespace net:[4026531969]
# ip addr
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN group default qlen 1
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast state UP group default qlen 1000
    link/ether 80:fa:5b:0b:a7:02 brd ff:ff:ff:ff:ff:ff
    inet 192.168.0.253/24 scope global eth0
       valid_lft forever preferred_lft forever
```

Sysadm can provide users with the ability to switch between their network
namespaces using **cado** This is the [Cado on GitHub](https://github.com/rd235/cado).

**netnsjoin** can join a network name space identified by:
  - the pid of a process
  - the net namespace id, i.e. something like net:[1234567890]
  - a tag defined by a namespace placeholder (see **nshold** below)

```
$net_admin# netnsjoin net:[4026532883] bash
Joining net namespace net:[4026532883]
$ netnslist -ss $$
net:[4026532883]
```

## nshold: a tool for namespace survival and tagging

A namespace survives until there is at least a process running in it (or if
it has been *mounted*, see mount(2))

**nshold** creates a namespace *placeholder*: 
an idle process whose cmdline is the namespace id
(so it can be easily found in the output of ps(1)).


```
$ pidnshold
$ ps ax | grep pid:
10199 ?        Ss     0:00 pid:[4026531836]
10205 pts/58   S+     0:00 grep pid:
```

It is also possible to assign a tag to a namespace using a placeholder.
```
$ netnshold safenet
$ ps ax | grep net:
10297 ?        Ss     0:00 net:[4026532883] safenet
10301 pts/58   S+     0:00 grep net:
```

User may like to add long tags consisting of several words. They can use
shell quoting, or a specific syntax provided by **nshold**
```
$ netnshold -- house automation
$ ps ax | grep house
10348 ?        Ss     0:00 net:[4026532883] house automation
10351 pts/58   S+     0:00 grep house
```

## nsrelease: kill a namespace placeholder

```
$ nsrelease pid:[4026531836]
```
or
```
$ nsrelease 10199
```
or
```
$ nsrelease safenet
```

It is also possible to kill all the namespaces whose tags match a spefic
regular expression.
For example the following command kills all the placeholders for uts
namespaces and defining four letter tags.
```
$ utsnsrelease -r "...."
```

## netnsjoin (again, now using tags!)

**netnsjoin** can join a namespace using the tag of a placeholder instead
of a pid.

```
$net_admin# netnsjoin safenet bash
Joining net namespace net:[4026532883]
$
```
or
```
net_admin# netnsjoin house automation -- bash
Joining net namespace net:[4026532883]
$
```

## nslist (again, now using tags!)

**nslist** can use tags and match tags using regular expressions.

```
$ nslist -ss safenet
net:[4026532883]
```

```
$ nslist -T -r house 
          NAMESPACE      PID CMDLINE
   net:[4026532883]    10297 net:[4026532883] safenet
   net:[4026532883]    10348 net:[4026532883] house automation
   net:[4026532883]     8782 -bash
   net:[4026532883]    10944 bash
   net:[4026532883]    11195 nslist -T -r house
```

Show all the net placeholders
```
$ netnslist -HT
          NAMESPACE      PID CMDLINE
   net:[4026532690]    11631 net:[4026532690] othernet
   net:[4026532883]    10297 net:[4026532883] safenet
   net:[4026532883]    10348 net:[4026532883] house automation
```

