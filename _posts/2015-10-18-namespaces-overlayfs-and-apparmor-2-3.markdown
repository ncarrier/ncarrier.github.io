---
layout: post
title:  "Namespaces, AppArmor and overlayfs - Part 2 / 3"
date:   2015-10-18 23:21:00
categories: namespaces apparmor overlayfs C linux
author: Nicolas Carrier
description: Playing with OverlayFS
---

Now that the firmware can be instantiated in it's own namespace, in it's own
root, we would like to be able to run multiple instances in parallel, without
having one modifying the file system of the other.
If possible, it would be great if we could keep the original firmware pristine
and have the diff between it before and after the run, easily identifiable.
This problem can be solved with union file systems in general and OverlayFS in
particular, which is the object of this post.

# Prerequisite

If you want to try the commands demonstrated in this post, you'll need a
**3.18+** linux kernel.
You can use the **linux-generic-lts-vivid** package if you use ubuntu 14.04 and
for example **the linux-image-4.2.0-0.bpo.1-amd64** on debian 8 from
**jessie-backports**.  

# Union filesystems

Union filesystems allow to mount multiple filesystems systems in a single mount
point, in a multi-layered fashion.
Various implementations have existed, the main ones being [UnionFS][unionfs],
[AuFS][aufs] and [OverlayFS][overlayfs].
The idea is to mount one or more read-only layers and on top of that, a
read-write one.
The user will see the content of all the layers merged when reading and when
doing modifications, only the top layer will be altered.
The first two one never made their way to the kernel, but OverlayFS did.
This is why, after giving a try to AuFS, which was included in distributions as
kernel patches, I used OverlayFS in firmwared.

# OverlayFS

OverlayFS is included in the kernel since 3.18.
It needs 4 directories to function properly: `lowerdir`, `upperdir`, `workdir`
and of course, the mount point, which we will call `union`.
For the sake of clarity, I will call `lowerdir` `ro` and `upperdir` `rw`.  
What's more, our aim is to use an already existing rootfs.
Two situations are possible, either the rootfs is contained in a directory, or
in a file system image.
We will demonstrate both situations.

First let's create a dummy file system image:

{% highlight bash %}
mkdir -p union ro rw workdir
dd if=/dev/zero of=rootfs.ext2 bs=1k count=32k
mkfs.ext2 rootfs.ext2 
sudo mount rootfs.ext2 rw/
sudo mkdir -p rw/foo rw/bar/baz
sudo sh -c " cat > rw/bar/test-file <<ThisIsAHereDocumentEndMarker
this a completely stupid test file
ThisIsAHereDocumentEndMarker"
sudo chmod -R a+w rw/* # this will save some sudos
sudo umount rw
{% endhighlight %}

Now that we have a file system, let's play with overlayfs.
First we mount the ext2 image as the readonly lowerdir layer:

{% highlight bash %}
sudo mount -oro rootfs.ext2 ro
{% endhighlight %}

Then we can mount the overlayfs:

{% highlight bash %}
sudo mount -t overlay -o lowerdir=ro,upperdir=rw,workdir=workdir overlay union/
{% endhighlight %}

The last **overlay** argument to the mount command, is normally the name of the
device to mount.
When it is not relevant, which is the case with OverlayFS, you are free to
choose it and you can see it as a label attached to the mount point.
It comes in handy when one want to filter the list of mount points, for example.

Now here we go:

{% highlight bash %}
touch union/bar/baz/42 # will alter rw only
echo "Really !" >> union/bar/test-file # test-write copied to rw then altered
tree
  .
  ├── ro
  │   ├── bar
  │   │   ├── baz
  │   │   └── test-file
  │   ├── foo
  │   └── lost+found [error opening dir]
  ├── rootfs.ext2
  ├── rw
  │   └── bar
  │       ├── baz
  │       │   └── 42
  │       └── test-file
  ├── union
  │   ├── bar
  │   │   ├── baz
  │   │   │   └── 42
  │   │   └── test-file
  │   ├── foo
  │   └── lost+found [error opening dir]
  └── workdir
      └── work [error opening dir]
cat ro/bar/test-file 
  this a completely stupid test file
cat rw/bar/test-file 
  this a completely stupid test file
  Really !
cat union/bar/test-file 
  this a completely stupid test file
  Really !
{% endhighlight %}

Then we can cleanup:

{% highlight bash %}
sudo umount union ro
rm -rf ro rootfs.ext2 rw union
sudo rm -rf workdir
{% endhighlight %}

# The problem with rw lower layers

But now let's consider a firmware developer's workflow.
He modifies it's code, recompiles it, generate the final (rootfs) directory and
then wants to test it's modifications.
Having to generate an ext2 file system and register it in firmwared (which
will have to compute it's sha1) would take an unreasonable amount of time.  
That's why **firmwared** allows to use directly a developer's final directory as
the firmware's `lowerdir` layer.  
But in this case, this layer is not readonly anymore.

The solution is simple, once your modifications are done, just ask linux to
remount the overlayfs mount point and it will reflect the changes done to the
`lowerdir`.

Let's get our hands dirty:

{% highlight bash %}
mkdir -p final ro rw union workdir
echo plop > final/greetings
sudo mount -oro --bind final/ ro/
sudo mount -t overlay -o lowerdir=ro,upperdir=rw,workdir=workdir overlay union/
cat union/greetings
  plop
rm final/greetings
echo moo > final/greetings
cat union/greetings
  plop
{% endhighlight %}

Here the greetings file contains 'plop' where it "should" contain 'moo'.
But after remounting union:

{% highlight bash%}
sudo mount -oremount union/
cat ro/greetings 
  moo
{% endhighlight %}

Now the content of greetings has been updated.
Please note that the `rm final/greetings` is important, if the file is the same
and only it's content has changed, then the remount will not be necessary.

And again, the cleanup.

{% highlight bash%}
sudo umount union ro
rm -rf ro final rw union
sudo rm -rf workdir
{% endhighlight %}

# Inside firmwared

Getting mount operations right in C programming is quite complicated, especially
for file system images.
What's more, the **mount** shell command is available, which is able of handling
all the filesystems and their options, currently implemented in Linux.
So the approach I took in firmwared was to implement a hook mechanism making
possible to mount, remount and unmount the union filesystems.  
The code is located in [hooks/mount.hook][mount_hook], it is written in bash.  
It's content should not give you headaches if you have followed the rest of the
post.
You can notice that `aufs` support is implemented too, but it is considered
deprecated and conflicts with the AppArmor implementation in firmwared.  
Note that by using hook scripts, it is really easy to add the support for other
file systems for the `lowerdir`, without firmwared even being aware of them.

# Partial conclusion and next step

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
The next and last post of the series will present the way one can implement it
"by hands", as I did in **firmwared**.

[aufs]: http://aufs.sourceforge.net/
[mount_hook]: https://github.com/Parrot-Developers/firmwared/blob/1be7f6f45f987fe43dd058021d67e6c7f21a5d39/hooks/mount.hook
[overlayfs]: https://www.kernel.org/doc/Documentation/filesystems/overlayfs.txt
[unionfs]:http://unionfs.filesystems.org/ 
