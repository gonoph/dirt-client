# dirt-client MUD Client
Originally based on `mcl` from Erwin S. Andreasen <erwin@andreasen.org>.

Forked by Bob McElrath <mcelrath@users.sourceforge.net> sometime in 2000.

## Summary

Dirt is a mud/muck/mush/moo/telnet client for linux with plugins for perl and
python scripting.  It is text-based and features: color, scrollback, history,
autocomplete, alias, trigger (even on ansi), zchat/mudmaster chat, mudftp,
MCCP, and a help system.

## History

### When I found it
Originally created by Erwin S. Andreasen, I became a fan of the software by
compiling and running it on my [Gentoo system][gentoo]. Sometime before 2008, it
was removed from the gentoo portage tree as it was no longer being maintained.
[Gentoo 1.4][gentoo_last] appears to be the last time I can find a reference to
`mcl` in the portage tree. Though, I do remember installing it from portage after
a later date.

[gentoo]: https://www.gentoo.org/
[gentoo_last]: https://distrowatch.com/table.php?distribution=gentoo&pkglist=true&version=1.4#pkglist

### Abandoned by it's Author
Sometime between Nov 20th, 2012 and Jan 2nd, 2013 (according to the [Way Back
Machine][mcl]), it had been abandoned by Mr Andreasen. Not only was the source
code not available, but the web page that described the project in the author's
own words was gone. It was as if all references to it disappeared within a
month's time. According to various change logs and commits, it doesn't look
like Mr Andreasen had updated the code in over a decade at this point.

### Forked by McElrath
According to the changelog in the `dirt.spec`, Bob McElrath forked `mcl` and
renamed it to `dirt` on Jan 26th, 2000 and placed it on [his domain][dirt].
However, this domain was abandoned sometime after Feb 14th, 2005. The domain
does not currently resolve. There is still a presence on
[SourceForge][sourceforge] for the project, but most of the files according to
[cvs rsync][rsync] haven't been updated since 2003, some source was updated in
2006, and some 3rd party scripts and contrib updated in 2012, and the source
forge project itself last updated in 2013.

[mcl]: https://web.archive.org/web/20121113103003/http://www.andreasen.org:80/mcl/
[dirt]: https://web.archive.org/web/20050214005930/http://draal.physics.wisc.edu:80/
[sourceforge]: https://sourceforge.net/projects/dirt-client/
[rsync]: http://dirt-client.cvs.sourceforge.net/

## Motivation for forking it myself
TL;DR: I wanted to compile it in 2018.

I stil have a lot of old MUD scripts that I had wrote, aliases for my favorite
MUD, and nostaglia for a program that enabled the MUD to consume a lot of my
spare time over the years. I've attempted several times to compile the code and
get it operational under a modern OS, but I kept running into road blocks which
I didn't have the time nor motivation to resolve.

Eventually I noticed that Mr Andreasen had abandoned the code base. It wasn't
until recently that I found the forked version from Dr McElrath. Thinking that
some of the harder work of modernizing the code base for a newer compiler had
mostly been done, I decided to try it out.

A quick glance of the compiler output and it was clear that it should work.
Granted, my first attempt required me to use 32-bit cross-compile, as the
config.guess script was so old that it would not recognize my `x86_64` system
as a valid architecture for the compile. I quickly decided that I would place
the project on GitHub, but that would require me to convert the CVS repository
into a git one. Thankfully, git has a plugin for this very thing, and it was
quick and painless.

I perform all my builds in a dedicated build enironment using OCI containers
(like docker), so I was able to check out what it would take to build
everything for a RHEL/CentOS 7 system, and then turn around and attempt to
build it in a Fedora 27 system. After a few hackish and messy edits here and
there, without changing too much how the program works, I now have a working
copy of the program compiled for a modern OS.

## Next Steps
I've kept the project mostly the same. I will be cleaning up some directory
structures, cleaning (fixing the warnings) the code, optimizing the spec file,
and eventually creating some automated build jobs for the RPMs. If you use the
program, and have some additional BUGS or ISSUES, please send them my way and
I'll see what I can do.

I've even thought about what it would look like to refactor the entire program
in another language. C/C++ was one of the 1st languages I learned on Unix
systems, so I don't tread lightly in that space. However, C/C++ has so many
nuances and differences across various platforms and architectures that it ends
up making a lot of the code into a macro hell. Perhaps a different language
would make the program more consistent and available across all the platforms
and architectures?

I'm not sure. Just something I was thinking about.

If you use this, let me know!

--b
Billy Holmes <gonoph>
