---
layout: post
title:  "Namespaces, AppArmor and overlayfs - Part 1 / 3"
date:   2015-10-18 17:52:00
categories: namespaces apparmor overlayfs C linux
author: Nicolas Carrier
---
The purpose of [firmwared], is to make a specially compiled firmware, (for a
 drone, for example) modified as little as possible, run on a desktop PC.
But if one wants to run a firmware's pid 1 (in our case, boxinit, an android
init's fork), he has to run it as root.
Then the problem is: how to protect the host PC from potentially harmful
behavior of the firmware ?  
In this series of three blog posts, I will present the approach I have taken in
firmwared, to attempt to solve this problem.

# Namespaces

[Linux namespaces][namespaces] are the base technology bricks, provided by the
linux kernel to implement containers, also called *lightweight virtualization*.
The key idea is that some resources, which are normally shared globally across
the processes of a system, can be *unshared* between parts of the pid tree.
Because these subtrees can't alter the original global resource, it is now
protected.  
More concretely, *chroot* can be viewed as the ancestor of linux namespaces.
Normally, all the processes share the same */* partition, but by calling the
[chroot] syscall, a process and all it's descendants can have another one, thus
protecting the rest of the file system which has now become unreachable.

Aside from *chroot*, 6 namespaces are implemented in the linux kernel, for
`IPC`, `network`, `mount tree`, `PIDs`, `user and groups` and `hostname and
domain name`.  

*Note:* The namespace for `hostname and domain name` is called *UTS*, which is
misleading: it does _not_ relate to time namespacing.
Protection against time modifications must be done with another mechanism, I
choose to use AppArmor for that, which I will detail in another post in this
series.

# Inside firmwared

I choose to implement containers directly by using the [unshare] syscall,
another option being to use the [clone] syscall, but I found it harder to get
well.

The interesting part lies in [launch_instance]. First, [setup_containers]
creates all of the namespaces we will use, excepted the `PID` one, more on this
later.

{% highlight c %}
flags = CLONE_FILES | CLONE_NEWIPC | CLONE_NEWNET |
		CLONE_NEWNS | CLONE_NEWUTS | CLONE_SYSVSEM;
unshare(flags);
mount(NULL, "/", NULL, MS_REC|MS_PRIVATE, NULL);
{% endhighlight %}

The `mount namespace` is special, once put in place, the mount hierarchy will
only be copied to the newnamespace, but will be still visible to the parent
namespace.
We want to avoid that, because if someone holds a reference on a file, the mount
point containing it can't be unmounted.
To prevent this, we indicate that the root of the container will be private to
it's members, recursively.

Firmwared then does some network interfaces configuration and proceeds to
[setup_chroot]:

{% highlight c %}
chroot(instance->union_mount_point);
chdir("/");
{% endhighlight %}

The `chdir` is needed to prevent processes escaping from their jail.

One has to be careful that once the chroot is set up, the executables
accessible, will be these of the rootfs.
So any configuration involving external helpers, for example,
`invoke_net_helper()` must take place before.

Then the process can `unshare(CLONE_PID)`, which will make the next `fork()`-ed
process, have a new `PID` namespace and a PID of 1, inside of it.
For this reason, this namespace must be created at the last moment, after all
the other forks have taken place, otherwise, the init process won't have a PID
of 1.


# Partial conclusion and next steps

Now that the firmware can be instantiated in it's own namespace, in it's own
root, we would like to be able to run multiple instances in parallel, without
having one modifying the file system of the other.
If possible, it would be great if we could keep the original firmware pristine
and have the diff between it before and after the run, easily identifiable.
This problem can be solved with union file systems in general and OverlayFS in
particular, which I will soon detail on the next blog post.

[chroot]: http://linux.die.net/man/2/chroot
[clone]: http://linux.die.net/man/2/clone
[firmwared]: https://github.com/ncarrier/firmwared-manifest
[launch_instance]: https://github.com/Parrot-Developers/firmwared/blob/1be7f6f45f987fe43dd058021d67e6c7f21a5d39/src/folders/instances.c#L494
[namespaces]: http://man7.org/linux/man-pages/man7/namespaces.7.html
[setup_containers]: https://github.com/Parrot-Developers/firmwared/blob/1be7f6f45f987fe43dd058021d67e6c7f21a5d39/src/folders/instances.c#L367
[setup_chroot]: https://github.com/Parrot-Developers/firmwared/blob/1be7f6f45f987fe43dd058021d67e6c7f21a5d39/src/folders/instances.c#L402
[unshare]: http://man7.org/linux/man-pages/man2/unshare.2.html
