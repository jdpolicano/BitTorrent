[![progress-banner](https://backend.codecrafters.io/progress/bittorrent/aaf93a7b-a45c-48f4-8e88-edfadae9f733)](https://app.codecrafters.io/users/jdpolicano?r=2qF)

This is a starting point for C solutions to the
["Build Your Own BitTorrent" Challenge](https://app.codecrafters.io/courses/bittorrent/overview).

In this challenge, you’ll build a BitTorrent client that's capable of parsing a
.torrent file and downloading a file from a peer. Along the way, we’ll learn
about how torrent files are structured, HTTP trackers, BitTorrent’s Peer
Protocol, pipelining and more.

**Note**: If you're viewing this repo on GitHub, head over to
[codecrafters.io](https://codecrafters.io) to try the challenge.

# Passing the first stage

The entry point for your BitTorrent implementation is in `app/main.c`. Study and
uncomment the relevant code, and push your changes to pass the first stage:

```sh
git add .
git commit -m "pass 1st stage" # any msg
git push origin master
```

Time to move on to the next stage!

# Stage 2 & beyond

Note: This section is for stages 2 and beyond.

1. Ensure you have `gcc` installed locally
1. Run `./your_bittorrent.sh` to run your program, which is implemented in
   `app/main.c`.
1. Commit your changes and run `git push origin master` to submit your solution
   to CodeCrafters. Test output will be streamed to your terminal.



# Local Development
TODOS:
- [x] Implement a binary safe dynamic string for use throughout the code. This can also be used as a dynamic byte array essentially.
- [x] Implement a set of url helpers for building a url for consumption in libcurl. This is necessary because libcurl doesn't actually support (surprisingly) building a url from a base url and adding query parameters to it.
- [ ] Create the skeleton of the application state machine. This will be used to manage the state of the application as it progresses through the various stages of the bittorrent protocol. Don't go overboard with this, just enough to get the basic functionality working. (we have no idea what challenges are coming up next, so we don't want to over-engineer this)
- [ ] tcp connection management. This will be used to manage the tcp connections to the tracker and the peers. This will be a simple wrapper around a socket and will somehow plug in with the application state to manage the back forth between the client, peers, and tracker (through libcurl).

