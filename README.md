# BitchX — Modernized

The legendary terminal IRC client, updated for 2026.

BitchX was the most popular terminal IRC client of the late 1990s and early 2000s. This fork modernizes it for current systems while preserving the original look, feel, and attitude.

## What's Changed (from BitchX 1.3)

- **TLS 1.3 Support** — Replaced deprecated `SSLv23_client_method()` with `TLS_client_method()` for OpenSSL 3.x compatibility
- **SASL PLAIN Authentication** — Connect and authenticate to modern IRC networks (Libera.chat, OFTC, DALnet)
- **Memory Leak Fix** — Fixed base64 output leak in SASL authentication flow
- **Compiles Clean** — Builds on Ubuntu 24.04, Debian 12, and modern Linux distributions

## Building

```bash
# Prerequisites
sudo apt install autoconf automake libssl-dev libncurses-dev build-essential

# Build
./autogen.sh
./configure --with-ssl
make -j$(nproc)

# Run
./source/BitchX -n YourNick
```

## Connecting to IRC

```
# Non-SSL
/server irc.dal.net

# SSL/TLS (port 6697)
/server -ssl irc.libera.chat 6697

# With SASL auth
# Format: server:+port:password:nick:network:saslnick:saslpass
/server irc.libera.chat:+6697:::Libera:yournick:yourpassword
```

## Roadmap

- [x] TLS 1.3 support (OpenSSL 3.x)
- [x] SASL PLAIN authentication
- [ ] Security hardening (sprintf → snprintf, strcpy → strlcpy)
- [ ] IRCv3 capability negotiation (message-tags, server-time, account-tag)
- [ ] Remove dead platform code (OS/2, Windows, GTK)
- [ ] Modern build system (CMake or Meson)
- [ ] UTF-8 support
- [ ] 256-color / true-color terminal support
- [ ] Updated documentation

## History

BitchX was originally created by panasync (Colten Edwards) in 1995 as a script pack for ircII, evolving into a full fork. It was the IRC client of choice for an entire generation of internet users. This fork aims to keep that legacy alive while making it work on modern systems.

## Contributing

Pull requests welcome. If you used BitchX back in the day and want to help modernize it, jump in. Check the roadmap above for what needs work.

## License

BSD License — see [COPYRIGHT](COPYRIGHT) for details.

Original authors: Colten Edwards (panasync) and contributors (1995-2012).
Modernization: Erik Anderson (2026).
