---
layout: post
title:  "Namespaces, AppArmor and overlayfs - Part 3 / 3"
date:   2015-10-26 07:20:00
categories: apparmor overlayfs C linux
longtitle: "Protect further the host, with AppArmor"
author: Nicolas Carrier
description: Protecting the host with AppArmor
---

Now our rootfs has it's file system which we'll be able to restore to it's
initial state with some umounts and rms.
It can also be shared between multiple instances.
What's more it is quite isolated from the rest of the system.
But the first things a firmware's pid 1 will do, will be to mount **/proc**,
**/sys** and **/dev** and by doing so, will gain access to potentially harmful
global resources.  
One of (the many) solutions involves a linux security module, such as SELinux or
AppArmor.
I choose to use the latter, first of all, because of it's shell like glob syntax
which is (sort of) human readable.
This post, last of the series, will present the way one can implement it "by
hands", as I did in **firmwared**.

The aim of this blog post is not to be a comprehensive guide on AppArmor.
It is rather an example of an implementation which is not "academic" in the
sense that the point of view is that of a developer, not of a sysadmin.

# Prerequisite

If you want to try the commands demonstrated in this post, you'll need to get
sure AppArmor is activated.
It is the case, by default on recent ubuntus (14.04 and 15.04 are fine) but not
in debian.
To enable AppArmor in debian, one can do :

        sudo perl -pi -e 's,GRUB_CMDLINE_LINUX="(.*)"$,GRUB_CMDLINE_LINUX="$1 apparmor=1 security=apparmor",' /etc/default/grub # I absolutely don't know what this command is doing, YHBW
        sudo update-grub

Then, after a reboot, to check AppArmor is now properly enabled, type :
`sudo aa-status`.
Or check the content of the `/sys/module/apparmor/parameters/enabled` file.

# Quick overview

[AppArmor][apparmor] allows to define profiles.
These profiles can be applied to a process and all it's descendants.
A profile defines things which are allowed and things which aren't.

For example, the following line :

        deny @{root}/union/proc/sysrq-trigger lrwx,

will `deny` read, write and execute (`rwx`) permissions on the file
`sysrq-trigger`.
Links will be forbidden too (`l`).
Note that it is possible to use variables, as it is the case here for `root`
which will only be known at runtime.

Other permissions can be granted, for instance, for using capabilities, like
`capability sys-chroot`, ...

The most precise source of information is the `apparmor.d` man page.
At the time of writing, the public AppArmor wiki mixes planned and actual
features of the syntax.

# Inside firmwared

The code is located in [src/apparmor.c][apparmor_c].
It uses a profile template, in
[resources/firmwared.apparmor.profile][apparmor_profile], used to generate the
real profile, based on information known at runtime, like the real root of the
instance.
When the instance is created, a custom profile is generated on-the-fly.
The [vload\_profile][vload_profile] function launches `apparmor_parser`
with `popen` to feed it's standard input with the profile.
Sadly, I couldn't find a function in libapparmor, to do the same thing.

Then when the instance is started, `aa_change_profile` is used to set the
profile used by the pid 1 process.

"Normal" use of AppArmor usually involves reusing profile pieces already
provided by the AppArmor distribution.
Here we wanted to control precisely the permissions, so all is contained in one,
simple profile.

# How the profile is built

The AppArmor philosophy in terms of security, is to use white lists, i.e., all
is forbidden by default.
It is understandable, for example, in a web context with aggressive hackers,
willing to compromise a system.
Here the risk is only involuntary harmful actions from a firmware.
So we choose to enable all and remove after, when problems are spotted.
Most of the capabilities have been granted.
Then all the file system accesses are allowed.
And at last, we deny accesses to some special files in `/proc`, `/sys` and
`/dev`.

A special note on `/proc/sysrq-trigger`, which allows to bypass the isolation
provided by the mount namespace with the `u` key, which asks linux to remount
all file systems read-only.
This "trick" is used by some init systems on shutdown and forces the user to
restart it's machine.

# When things go wrong

Two things can possibly go wrong with AppArmor in firmwared.

First, the firmware can be granted a permission it shouldn't have.
It happened with the `sys_time` capability: a firmware using it can change the
current date, leading to extremely irritating build issues...
In this case, one just need to remove the corresponding permission from the
profile.

Second, the firmware can be lacking a permission necessary for it's execution.
In this case, the `auditd` daemon comes in handy.
It is able to log all the accesses `denied` to the firmware and show which
permission should be granted.

The most efficient way to detect unduly blocked access is to use this one liner,
as root, which will add red color on DENIED logs and green on AUDIT ones:

        esc=$(printf '\033'); stdbuf -oL tail -f /var/log/audit/audit.log | stdbuf -oL sed "s/DENIED/${esc}[7;31mDENIED${esc}[0m/g" | stdbuf -oL sed "s/AUDIT/${esc}[1;32mAUDIT${esc}[0m/g" 

It uses sed to add green and red ansi escape sequences around the relevant words
and forces with stdbuf, the output to be line buffered, so that logs are
displayed line by line and not by blocks

# Conclusion of this post series

We have seen the actions taken to create a relative isolation between a process
and it's descendants in firmwared.
A lot more could be done to push further the control on the instances'
execution.
`Cgroups` for example, would allow to install quotas, permissions or even to
freeze all the instance's process hierarchy.
But for now, the important thing to note is that unlike containers like
solutions like docker or rkt, we don't want to protect ourselves against
malicious attackers.
We rather want to protect against potentially harmful behavior of pieces of
software persuaded to be alone on board.


[apparmor]: http://wiki.apparmor.net/index.php/Main_Page
[apparmor_c]: https://github.com/Parrot-Developers/firmwared/blob/98dd5d347767d3dc5d8ed0bb3377f27d6284fd26/src/apparmor.c
[apparmor_profile]: https://github.com/Parrot-Developers/firmwared/blob/1bd8cc2ddc86596ad6cb734c907584d74ad87d2e/resources/firmwared.apparmor.profile
[vload_profile]: https://github.com/Parrot-Developers/firmwared/blob/98dd5d347767d3dc5d8ed0bb3377f27d6284fd26/src/apparmor.c#L75
