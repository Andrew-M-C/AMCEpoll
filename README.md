# Project Briefing
A Linux epoll package tool. This project aims to provide libevent-type API with reduced memory use.

# Current Progress

I have successfully implemented file events and signal events. Both types of events support not only general read/write events, but also timeout events.

# Next Step to Do

In [develop](https://github.com/Andrew-M-C/AMCEpoll/tree/develop) branch, I am working on raw timeout event support. That is actually quite easy, as I already implemented them for more complex events. However, I am quite busy these days, because I am about to leave my current company for another.

# Protential Bug

When I was testing [this commit](https://github.com/Andrew-M-C/AMCEpoll/commit/7af31f7fdb2715a61cbe1d208ff492b5b5d3cbee) on my 64-Bits Ubuntu machine, an strange error "**stack smashing detected**" was raised. I could not find anywhere looks like faulty memory access in the project. As a result, I added a CFLAGS option "`-fno-stack-protector 
`" to advoid that. But I am not sure if this may lead to a bug.


