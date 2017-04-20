# Linux-epoll-tool
A Linux epoll package tool. This project aims to provide libevent-type API with reduced memory use.

# Current Progress

`epoll_wait()` invoke was completed. You can add an file descriptor to the event base so that you can handle events on this file descriptor in the callback you specified.

# Next Step to Do

In [develop](./tree/develop) branch, I am working on signal event support. 

# Future Features to Do

After implementing signal events, I want to support timeout events in AMCEpoll. Besides, file descriptor timeout events would be added then.


