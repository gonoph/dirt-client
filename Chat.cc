#include "dirt.h"
#include "cui.h"
#include "Chat.h"
#include <stdarg.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <net/if.h>
#include "Interpreter.h"

ChatServerSocket *chatServerSocket;
MUD *chatMUD;
static int flagLookup(const char *flag);
static const char* adjustFlags(const char *flags, int &flag);
static const char *flagString(int,bool);


#define CDEBUG (config->getOption(opt_chat_debug))
#define CHAT_NAME (config->getStringOption(opt_chat_name))

static void writeChat(const char *fmt, ...) __attribute__((format(printf,1,2)));

static void writeChatCommon(char *s) {
    sprintf(s+strlen(s), "%c%c\n", SET_COLOR, fg_white|bg_black);
    
    // Actions should probably be in some "chat" MUD
    char out[MAX_MUD_BUF];
//    if (embed_interp->eval("hook_run('output')", s, out))
    strcpy(out, s);
    hook.run(OUTPUT, out);
        outputWindow->print(out);
//    else
//        outputWindow->print(s);

}

// Write the message with some prefix so we can easier distinguish the chat messages from normal stuff
static void writeChat(const char *fmt, ...) {
    char buf[MAX_MUD_BUF];
    char *s = buf+sprintf(buf, "%c%cCHAT:%c%c ", SET_COLOR, bg_black|fg_green|fg_bold, SET_COLOR, config->getOption(opt_chat_syscolor));
    va_list va;
    
    va_start(va, fmt);
    s += vsnprintf(s, sizeof(buf) - (s-buf) - 3, fmt, va); // -1 for the \n, 2 for the color reset
    va_end(va);

    writeChatCommon(buf);
}

// As above, but this is for the actual talking rather than system messages
static void writeChatText(const char *fmt, ...) {
    char buf[MAX_MUD_BUF];
    char *s = buf+sprintf(buf, "%c%cCHAT:%c%c ", SET_COLOR, bg_black|fg_green|fg_bold, SET_COLOR, config->getOption(opt_chat_chatcolor));
    va_list va;
    
    va_start(va, fmt);
    s += vsnprintf(s, sizeof(buf) - (s-buf) - 3, fmt, va); // -1 for the \n, 2 for the color reset
    va_end(va);

    writeChatCommon(buf);
}

// clean up escape chars, return at most new_len bytes
static const char* sanitize(const char *data, int len, int new_len) {
    StaticBuffer buf(new_len+1);
    char *out = buf;
    char *end = buf+new_len-4;
    
    while (isspace(*data) && len > 0) { // spaces/newlines at the beginning
        data++; len--;
    }
    
    while (len > 0 && isspace(data[len-1]))
        len--;
    
    ColorConverter col;
    
    while (len > 0 && out < end) {
        const char *color_start;
        
        if (*data == '\e') {
            color_start = data;
            while(len > 0 && *data != 'm') {
                data++;
                len--;
            }
            
            if (*data == 'm') {
                int color = col.convert((byte*)color_start, data-color_start);
                if (color) {
                    *out++ = SET_COLOR;
                    *out++ = color;
                }
                data++;
                len--;
                continue;
            }
        }

        // ignore 0240 (mudmaster soft newline?) and any other control characters
        if ((byte)*data == 0240 || (byte)*data < 32)
            ;
        else if ((byte)*data > 127)
            out += sprintf(out, "\\%03o", (byte)*data);
        else
            *out++ = *data;

        data++;
        len--;
    }
    
    
    *out++ = NUL;
    return buf;
}

ChatServerSocket::ChatServerSocket() {
    is_afk = false;
    generateId();
}

const char * ChatServerSocket::findIPAddress() {
    char intf[INPUT_SIZE];
    const char *interfaces = config->getStringOption(opt_chat_interfaces);
    struct ifreq interface;
    
    do {
        interfaces = one_argument(interfaces, intf, true);
        if (intf[0]) {
            strcpy(interface.ifr_name, intf);
            if (ioctl (getFD(), SIOCGIFADDR, &interface) < 0) // maybe it doesn't exist
                continue;

            struct sockaddr_in sock= *((struct sockaddr_in*)&interface.ifr_addr);
            
            if (ioctl(getFD(), SIOCGIFFLAGS, &interface) < 0)
                return NULL;
            
            if (interface.ifr_flags & IFF_UP) {
                return inet_ntoa(sock.sin_addr);
            }
        }
    } while (intf[0]);
    
    return NULL;
}

ChatConnection* ChatServerSocket::findByAddress(const char *ip, int port) {
    FOREACH(ChatConnection*,c,connections)
        if (!strcmp(c->getRemoteServerIP(),ip) && c->getRemoteServerPort() == port)
            return c;
    return NULL;
}

ChatConnection* ChatServerSocket::findConnection(const char *name) {
    // Let's try to compare the names first
    FOREACH(ChatConnection*,c,connections)
        if (c->getName() == name || !strcmp(c->getRemoteServerIP(), name))
            return c;

    // Number?
    int n = atoi(name);
    if (n > 0)
        return connections[n-1];
    
    return NULL;
}

void ChatServerSocket::generateId() {
    id = random() ^ getpid() ^ current_time;
}

ChatServerSocket::~ChatServerSocket() {
    FOREACH(ChatConnection*,c,connections)
        delete c;
}

void ChatServerSocket::idle() {
    FOREACH(ChatConnection*,c,connections)
        c->idle();
    
    if (!is_afk && tty->last_activity + 300 < current_time) {
        handleUserCommand("afk", "quiet");
    } else if (is_afk && tty->last_activity + 5 > current_time) {
        handleUserCommand("afk", "quiet");
    }
}

void ChatServerSocket::connectionAccepted(int fd, struct sockaddr_in *remote) {
    ChatConnection *c = new ChatConnection(fd,remote);
    connections.insert(c);
    c->handleIncoming();
}

void ChatServerSocket::call(const char *name, int port, Protocol protocol) {
    ChatConnection *c = new ChatConnection();
    connections.insert(c);
    c->connect(name, port, protocol);
}

void ChatServerSocket::handleUserCommand(const char *name, const char *arg) {
    if (!strcmp(name, "call")) {
        char hostname[MAX_INPUT_BUF];
        char port_buf[MAX_INPUT_BUF];
        int protocol = -1, port;
        
        arg = one_argument(arg, hostname, true);
        if (!strcmp(hostname, "zchat"))
            protocol = zchat;
        else if (!strcmp(hostname, "chat"))
            protocol = mudmaster;
        
        if (protocol != -1)
            arg = one_argument(arg, hostname, true);
        
        arg = one_argument(arg, port_buf, true);
        if (!hostname[0])
            writeChat("You must provide a hostname/nickname to call");
        else {
            if (!port_buf[0])
                port = 4050;
            else
                port = atoi(port_buf);
            if (port < 1 || port > 65535)
                writeChat("Invalid port number: %d", port);
            
            // What protocol do we use?
            if (protocol == -1) {
                if (config->getOption(opt_chat_protocol) == 0)
                    protocol = mudmaster;
                else
                    protocol = zchat;
            }
            
            // Let's reget our IP address
            const char *address = findIPAddress();
            if (address && strcmp(address, ~ip)) {
                writeChat("Warning: it looks like our IP address is now %s", address);
                ip = address;
            }
            
            call(hostname, port, (Protocol)protocol);
        }
        
    } else if (!strcmp(name, "ip")) {
        computeIPAddress();
        
    } else if (!strcmp(name, "group")) {
        char group_name[MAX_INPUT_BUF];
        arg = one_argument(arg, group_name, true);
        if (!group_name[0])
            writeChat("Chat to what group?");
        else if (!arg[0])
            writeChat("Chat what to the '%s' group?", group_name);
        else {
            bool emote = false, ok = false;
            if (!strncasecmp(arg, "emote ", 6)) {
                emote = true;
                arg += 6;
            }

            FOREACH(ChatConnection*,c,connections)
                if (c->getGroup() == group_name) {
                    ok = true;
                    c->sendTextGroup(group_name, arg, emote);
                }

            const char *colored = sanitize(arg,strlen(arg), 1024);
            
            if (!ok)
                writeChat("No connected members of group '%s'", group_name);
            else {
                if (emote)
                    writeChat("[emote to %s] %s %s%c%c", group_name, ~CHAT_NAME, colored, SET_COLOR, config->getOption(opt_chat_chatcolor));
                else
                    writeChat("You chat to %s: '%s%c%c'", group_name, colored, SET_COLOR, config->getOption(opt_chat_chatcolor));
            }
        }
    } else if (!strcmp(name, "setgroup")) {
        char group_name[MAX_INPUT_BUF];
        arg = one_argument(arg, group_name, true);
        if (!group_name[0])
            writeChat("Add/remove members to what group?");
        else if (!arg[0])
            writeChat("Add/remove what members to group %s?", group_name);
        else {
            char name[MAX_INPUT_BUF];
            arg = one_argument(arg, name, true);
            while(name[0]) {
                ChatConnection *c = findConnection(name);
                if (!c)
                    writeChat("No such connection: %s", name);
                else if(c->getGroup() == group_name) {
                    c->setGroup("");
                    writeChat("%s now in no group", c->shortDescription());
                } else {
                    c->setGroup(group_name);
                    writeChat("%s now in group %s", c->shortDescription(), group_name);
                }
                arg = one_argument(arg, name, true);
            }
        }
    } else if (!strcmp(name, "request")) {
        if (!arg[0])
            writeChat("%cchat.request ALL|<connection name>", CMDCHAR);
        else if (!strcmp(arg, "all")) {
            FOREACH(ChatConnection*,c,connections) {
                c->sendCommand(cmdRequestConnections, "");
                c->setFlags(c->getFlags() | flagRequestPending);
            }
        } else {
            ChatConnection *c = findConnection(arg);
            if (!c)
                writeChat("No such connection '%s'", arg);
            else {
                c->sendCommand(cmdRequestConnections, "");
                c->setFlags(c->getFlags() | flagRequestPending);
            }
        }
        
    } else if (!strcmp(name, "peek")) {
        if (!arg[0])
            writeChat("%cchat.peek ALL|<connection name>", CMDCHAR);
        else if (!strcmp(arg, "all")) {
            FOREACH(ChatConnection*,c,connections) {
                c->sendCommand(cmdPeekConnections, "");
                c->setFlags(c->getFlags() | flagRequestPending);
            }
            
        } else {
            ChatConnection *c = findConnection(arg);
            if (!c)
                writeChat("No such connection '%s'", arg);
            else {
                c->sendCommand(cmdPeekConnections, "");
                c->setFlags(c->getFlags() | flagRequestPending);
            }
        }
        
    } else if (!strcmp(name, "ping")) {
        if (!arg[0])
            writeChat("%cchat.ping ALL|<connection name>", CMDCHAR);
        else if (!strcasecmp(arg, "all")) {
            FOREACH(ChatConnection*, c, connections)
                c->sendPing();
        } else {
            ChatConnection *c =  findConnection(arg);
            if (!c)
                writeChat("No such connection: %s", arg);
            else
                c->sendPing();
        }
        
    } else if (!strcmp(name, "list")) {
        int i = 1;
        bool verbose = (outputWindow->width > 90);
        FOREACH(ChatConnection *, c, connections)
            outputWindow->printf("%2d %s\n", i++, c->longDescription(verbose));
        outputWindow->printf("\n%d active connections.\n", i-1);
        
        
    } else if (!strcmp(name, "accept")) {
        bool ok = false;
        
        FOREACH(ChatConnection *, c, connections)
            if (c->acceptConnection()) {
                ok = true;
                break;
            }
        if (!ok)
            writeChat("No connections are waiting for your accept.\n");
        
    } else if (!strcmp(name, "snoop")) {
        if (!arg[0])
            writeChat("Snoop whom?");
        else {
            ChatConnection *c = findConnection(arg);
            if (!arg[0])
                writeChat("No such connection as '%s'", arg);
            else {
                c->sendCommand(cmdSnoop, "");
            }
        }
        
    } else if (!strcmp(name, "to")) {
        char name[MAX_INPUT_BUF];
        arg = one_argument(arg,name, false);
        
        if (!name[0] || !arg[0])
            writeChat("Chat what and to whom?");
        else {
            ChatConnection *c = findConnection(name);
            if (!c)
                writeChat("No such connection: %s", name);
            else {
                if (!strncasecmp(arg, "emote ", 6)) {
                    arg += 6;
                    c->sendTextPersonal(arg, true);
                } else
                    c->sendTextPersonal(arg, false);
                
            }
        }
        
    } else if (!strcmp(name, "all")) {
        if (!arg[0])
            writeChat ("Chat WHAT to all?");
        else {
            bool emote = false;
            if (!strncasecmp(arg, "emote ", 6)) {
                emote = true;
                arg += 6;
            }

            const char *colored = sanitize(arg,strlen(arg), 1024);
            
            FOREACH(ChatConnection*,c,connections)
                c->sendTextEverybody(arg, emote);
            
            if (!emote)
                writeChatText("You chat to all: '%s%c%c'", colored, SET_COLOR, config->getOption(opt_chat_chatcolor));
            else
                writeChatText("[emote to all] %s %s%c%c.", ~CHAT_NAME, colored, SET_COLOR, config->getOption(opt_chat_chatcolor));
        }
        
    } else if (!strcmp(name, "icon")) {
        if (access(arg, R_OK) != 0)
            writeChat("Can't find %s: %m",arg);
        else {
            writeChat("chat.icon updated to %s and sent", arg);
            FOREACH(ChatConnection*,c,connections)
                c->sendIcon(arg);
        }
        
    } else if (!strcmp(name, "reject")) {
        bool ok = false;
        
        FOREACH(ChatConnection *, c, connections) {
            const char *desc = c->shortDescription();
            if (c->rejectConnection()) {
                ok = true;
                writeChat("Connection from %s rejected", desc);
                break;
            }
        }
        
        if (!ok)
            outputWindow->printf("No connections are waiting for your accept.\n");
        
    } else if (!strcmp(name, "afk")) { // FIXME this doesn't seem to work...always says "is now AFK"
        is_afk = !is_afk;
        if (is_afk)  {
            if (strcmp(arg, "quiet"))
                writeChat("You are now AFK");
        }
        else {
            if (strcmp(arg,"quiet"))
                writeChat("You are no longer AFK");
        }
        FOREACH(ChatConnection*,c,connections)
            c->sendStatus();
        
    } else if (!strcmp(name, "flags")) {
        char name[MAX_INPUT_BUF];
        arg = one_argument(arg, name, true);
        if (!arg[0])
            writeChat("Set flags to what?");
        else {
            ChatConnection * c = findConnection(name);
            if (!c)
                writeChat("No such connection as '%s'", name);
            else {
                int flags = c->getFlags();
                const char *res = adjustFlags(arg, flags);
                if (res)
                    writeChat("Error setting flags: %s", res);
                else {
                    writeChat("Flags for %s set to %s", c->shortDescription(), flagString(flags, false));
                    c->setFlags(flags);
                }
                
            }
        }
    }
    else
        status->setf("Unknown chat command: chat.%s", name);
}

void ChatServerSocket::computeIPAddress() {
    const char *addr = findIPAddress();
    if (!addr) {
        writeChat("System active, but could not determine IP address. Port %d", getLocalPort());
        embed_interp->set("chatIP", "");
    } else {
        writeChat("System active. Listening on %s port %d", addr, getLocalPort());
        ip = addr;
        embed_interp->set("chatIP", addr);
    }
}

void ChatServerSocket::handleSnooping(const char *data, int len) {
    FOREACH(ChatConnection *,c,connections)
        if (c->getFlags() & flagSnoopedBy)
            c->sendCommand(cmdSnoopData,data,len);
}

void ChatServerSocket::create(int base_port) {
    int port,res=0;
    ChatServerSocket *s = new ChatServerSocket;
    
    for (port = base_port; port < base_port+10; port++) {
        res = s->bind(port);
        if (res == EADDRINUSE)
            continue;
        else if (res == 0)
            break;
    }
    
    if (res == EADDRINUSE)
        writeChat("Cannot bind chat port, all ports %d to %d are in use", base_port, base_port+9);
    else if (res != 0)
        writeChat("Error ocurred during chat port bind: %s", strerror(res));
    else { // OK
        s->computeIPAddress();
        embed_interp->set("chatPort", s->getLocalPort());
        chatServerSocket = s;
        return;
    }
    
    delete s;
}


ChatConnection::ChatConnection (int fd, struct sockaddr_in *remote) : Socket(fd,remote) {
    state = invalid;
    flags = 0;
    inactive_since = 0;
    pgpKey = NULL;
    person_status = active;
    
    if (remote) {
        accepted = true;
        connect_time = current_time;
    }
    else
        accepted = false;
}

ChatConnection::~ChatConnection() {
    chatServerSocket->connections.remove(this);
    delete[] pgpKey;
}

void ChatConnection::idle() {
    if (state == connecting && current_time >= timeout)
        close ("connect timeout");
    else if (state == requesting && current_time >= timeout)
        close ("negotation timeout");
}

const char* ChatConnection::shortDescription() const {
    return Sprintf("%s@%s", name.len() ? ~name : "?", !strcmp(getRemoteIP(), "0.0.0.0") ? "<none>" : getRemoteIP());
}

void ChatConnection::close(const char *reason) {
    writeChat("Closing connection to %s (%s)", shortDescription(), reason);
    delete this;
}

void ChatConnection::handleIncoming() {
    if (CDEBUG) writeChat("Incoming connection from %s", shortDescription());
    state = incoming_chatname;
    // Now wait for the request
    // @@ Handle connection spamming here
}

void ChatConnection::connectionEstablished() {
    state = requesting;
    timeout = current_time + 60;
    ip = getRemoteIP();
    if (CDEBUG) writeChat("Connected to %s, now negotating", shortDescription());
    requestConnection();
}

void ChatConnection::connect (const char *hostname, int port, int _protocol) {
    writeChat ("Trying to connect to %s port %d%s", hostname, port, _protocol == mudmaster ? " (mudmaster)" : "");
    state = connecting;
    int res;
    
    if ((res = Socket::connect(hostname, port, true)) != 0) {
        writeChat("Error connecting to %s: %s", hostname, res == ENOENT ? "No such host" : strerror(res));
        delete this;
    } else {
        timeout = current_time + 60;
        remote_port = port;
    }

    protocol = (Protocol)_protocol;
}

void ChatConnection::errorEncountered(int) {
    writeChat("Error ocurred while talking to %s: %s", shortDescription(), getErrorText());
    delete this;
}

const char* ChatConnection::longDescription(bool verbose) {
    StaticBuffer buf;
    char *s = buf;
    
    s += sprintf(s, "%c%c%-12s ", protocol == zchat ? 'Z' : ' ', accepted ? '<' : '>', name.len() ? ~name : "<unknown>");
    
    if (ip.len()) {
        s += sprintf(s, " %16s %4d ", ~ip, remote_port);
    } else {
        s += sprintf(s, " %16s %4d ", getRemoteServerIP(), getRemoteServerPort());
    }
    
    if (verbose)
        s += sprintf(s, "%-14.14s ", ~version);
    
    if (verbose) {
        if (person_status == afk  || person_status == inactive)
            s += sprintf(s, person_status == afk ? "AFK:%-3ld" : "Inc:%-3ld", (current_time - inactive_since)/60);
        else
            s += sprintf(s,  person_status == inactive ? "inact" : "     ");
    } else {
        s += sprintf(s, "%c", person_status == afk ? 'A' : person_status == inactive ? 'I' : ' ' );
    }

    s += sprintf(s, "%s ", flagString(flags, true));

    if (group.len())
        s += sprintf(s, "[%s]", ~group);
    
    if (state == invalid)
        s += sprintf(s, verbose ? "INVALID STATE" : "????");
    else if (state == connecting)
        s += sprintf(s, verbose ? "Connecting (timeout: %lds)" : "C%.3ld", timeout - current_time);
    else if (state == requesting)
        s += sprintf(s, verbose ? "Connected, requesting permission (timeout: %lds)" : "P%.3ld", timeout-current_time);
    else if (state == connected)
        s += sprintf(s, verbose ? "Connected" : "    ");
    else if (state == waiting_command_header)
        s += sprintf(s, verbose ? "Waiting for %d more command header bytes" : "Cmd%d" , 4-command_header_received);
    else if (state == waiting_command_data) {
        if (verbose)
            s += sprintf(s, "Waiting for %d more bytes of command %d", data_length, pending_command);
        else
            s += sprintf(s, "D%.3x", data_length);
    }
    else if (state == waiting_data)
        s += sprintf(s, verbose ? "Waiting for old-style data" : "Data");
    else if (state == incoming_chatname)
        s += sprintf(s, verbose ? "Accepted, awaiting protocol/chatname" : "Acc1");
    else if (state == incoming_address)
        s += sprintf(s, verbose ? "Accepted, awaiting address/port" : "Acc2");
    else if (state == awaiting_confirmation) {
        if (verbose)
            s += sprintf(s, "Waiting for your accept (%cchat.accept)", CMDCHAR);
        else
            s += sprintf(s, "Cnfr");
    }
    else {
        s += sprintf(s, verbose ? "Unknown state: %d" : "I%d", state);
    }

    
    return buf;
}


void ChatConnection::inputReady() {
    assert (state != invalid);
try_again:
    if (pendingInput() <= 0)
        return;
    
    // Two states require us to read some lines
    if (state == incoming_chatname) {
        const char *line = readLine();
        if (line) {
            char protocol_name[64], chat_name[64], chat_id[64];
            if (!strncmp(line, "CHAT", strlen("CHAT"))) { // MudMaster chat protocol
                protocol = mudmaster;
                if (2 != sscanf(line, "%64[^:]:%64[^\n]", protocol_name, chat_name)) {
                    close(Sprintf("Invalid MudMaster protocol line: \"%-.128s\"", line));
                    return;
                }
                chat_id[0] = NUL;
            } else if (!strncmp(line, "ZCHAT", strlen("ZCHAT"))) { // Zugg's extensions
                protocol = zchat;
                if (3 != sscanf(line, "%64[^:]:%64[^\t]\t%64[^\n]", protocol_name, chat_name, chat_id)) {
                    close(Sprintf("Invalid ZCHAT protocol \"line: %-.128s\"", line));
                    return;
                }
            } else {
                close(Sprintf("Invalid protocol: : %-.128s", line));
                return;
            }
            
            name = chat_name;
            id = chat_id;
            
            if (CDEBUG) writeChat("%s: uses protocol %s", shortDescription(), protocol_name);
            
            // Wait for hostname/port
            state = incoming_address;
            goto try_again;
        }
    } else if (state == incoming_address) {
        const char *line;
        char linebuf[1024];

        // mudmaster doesn't seem to send a newline at the end of the address line.
        if (protocol == mudmaster) {
            int count = read(linebuf,sizeof(linebuf));
            if (count <= 0)
                return;
            
            linebuf[count] = NUL;
            line = linebuf;
        } else {
            line = readLine();
        }
        
        if (line) {
            char ip_address[128], port_number[128];
            const char *port_start;
            if (strlen(line) < 12 || strlen(line) > 63)
                close(Sprintf("Line with IP address/port impossibly short or long: %-.128s", line));
            else {
                port_start = line+strlen(line) - 5; // grr, why this weirdness
                strncpy(ip_address, line, port_start-line);
                ip_address[port_start-line] = NUL;
                
                strcpy(port_number, port_start);
                remote_port = atoi(port_number);
                
                if (remote_port  < 1 || remote_port > 65535) {
                    close(Sprintf("Impossible port number %d", remote_port));
                    return;
                }
                
                if (inet_addr(ip_address) == (unsigned int)-1) {
                    close (Sprintf("Impossible IP address: %s", ip_address));
                    return;
                }
                ip = ip_address;
                
                // Problem: the current version of the zChat specification leaves no delimiter at the end of the
                // security info section. Zugg suggests just to read whatever has arrived so far and hope for the best
                // a separrator will appear in the next version of zChat...
                if (protocol == zchat) {
                    char security_buf[MAX_MUD_BUF];
                    /* int n =*/ read(security_buf, sizeof(security_buf));
                    // Check passphrase
                }
                
                state = awaiting_confirmation;
                
                if (config->getOption(opt_chat_nodisturb))
                    rejectConnection();
                // Accept all connections?
                else if (config->getOption(opt_chat_autoaccept))
                    acceptConnection();
                // Accept connection based on security phrase/name
                else {
                    writeChat("%s has requested a connection. Use %cchat.accept to allow it or %cchat.reject to drop it", shortDescription(), CMDCHAR, CMDCHAR);
                }
            }
        }
    } else if (state == connecting) {
        close("Received data while still connecting?!");
    } else if (state == connected) {
        // A command!
        if (protocol == zchat) {
            state = waiting_command_header;
            command_header_received = 0;
            goto try_again;
        } else {
            // One byte-command
            char cmd;
            int count = read(&cmd, 1);
            assert (count == 1);
            pending_command = cmd;
            // Now get the data, look out for the 255 character.
            state = waiting_data;
            goto try_again;
        }
    } else if (state == waiting_data) {
        char *buf = command_data.get(4096);
        int n = read(buf, 4096);
        char *eoc = (char*)memchr(buf, 255, n);
        if (eoc) {
            int count = eoc-buf;
            if (count < n)
                unread(eoc+1, n - count - 1); // return that many bytes back to the socket buffers
            command_data.use(eoc-buf); // skip the terminator
            dispatchCommand(pending_command, ~command_data, command_data.count());
            command_data.clear();
            state = connected;
        }
        else {
            // otherwise, continue wating for the character
            command_data.use(n);
        }
        
    } else if (state == waiting_command_header) {
        int n = read(command_header+command_header_received, 4-command_header_received);
        assert (n > 0);
        command_header_received += n;
        assert (command_header_received <= 4);
        if (command_header_received == 4) {
            pending_command = command_header[0] + 256 * command_header[1];
            data_length = command_header[2] + 256 * command_header[3];
            state = waiting_command_data;
            if (data_length == 0) { // no data, dispatch right away
                state = connected;
                dispatchCommand(pending_command, ~command_data, command_data.count());
                command_data.clear();
            } else {
                goto try_again;
            }
        }
    } else if (state == waiting_command_data) {
        if (data_length > 0) {
            char *s = command_data.get(data_length);
            int n = read(s, data_length);
            data_length -= n;
            command_data.use(n);
        }
        if (data_length == 0) {
            // Got all the data now
            state = connected;
            dispatchCommand(pending_command, ~command_data, command_data.count());
            command_data.clear();
            goto try_again;
        }
        
    } else if (state == requesting) { // waiting for YES/NO response
        const char *line = readLine();
        if (line) {
            if (!strncasecmp(line, "NO", 2))
                close("Remote reject connection attempt");
            else if (!strncasecmp(line, "YES:", 4)) {
                state = connected;
                char username[128];
                if (1 != sscanf(line, "YES:%s\n", username))
                    close ("No name specified in initial connection");
                else {
                    name = username;
                    writeChat("%s accepted our chat connection", shortDescription());
                    sendAll();
                    goto try_again;
                }
            } else {
                close(Sprintf("Strange response: %.64s not YES/NO", line));
            }
        }
    }
}

void ChatConnection::sendCommand(int command, const char *data, int len) {
    if (len == -1)
        len = strlen(data);

    if (CDEBUG) writeChat("Sending command %d: %d bytes of data", command, len);
    if (CDEBUG) writeChat("Data: '%s'", sanitize(data, len, 256));
    
    if (protocol == zchat) {
        char buf[4];
        buf[0] = command % 256;
        buf[1] = command / 256;
        buf[2] = len % 256;
        buf[3] = len / 256;
        write(buf, sizeof(buf));
        if (len  > 0)
            write(data,len);
    } else { // 255 terminated MudMaster command
        assert(memchr(data, 255, len) == NULL); // Hmm, how IS it done
        write(Sprintf("%c", command), 1);
        write(data, len);
        write("\377", 1);
    }
}

// Return some timestamp in miliseconds. This wraps at 1 million
static int getMiliseconds() {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) < 0)
        return 0;
    else
        return tv.tv_sec % 1000 + (tv.tv_usec / 1000);
}

void ChatConnection::dispatchCommand (int command, const char *data, int len) {
    int stamp_id = -1;
    // Extract 4-byte stamp ID from zChat
    if (protocol == zchat && (command == cmdTextPersonal || command == cmdTextGroup || command == cmdTextEverybody)) {
        if (len < 5) {
            writeChat("Command %d less than 5 bytes long; cannot have stampID and sufficient text", command);
            return;
        }
        // Byte order ?! Assume raw
        stamp_id = *(int*)data;
        data += 4;
        len -= 4;
        
        if (stamp_id == chatServerSocket->getId()) {
            if (CDEBUG) writeChat("Ignoring duped message: %s", sanitize(data,len,1024));
            return;
        }
    }
    
    const char *sane = sanitize(data, len, 1024); // how much is too much?

    if (CDEBUG) writeChat("[%s] %d = %s", shortDescription(), command, sane);
        
    
    switch(command) {
        
    case cmdTextPersonal:
        if (!(flags & flagIgnored)) {
            if (config->getOption(opt_chat_paranoia) || !strstr(sane, ~name))
                writeChatText("[%s] %s%c%c", shortDescription(), sane);
            else
                writeChatText("%s", sane);
        }
        break;
        
    case cmdMessage:
        writeChat("[%s] %s", shortDescription(), sane);
        break;
        
    case cmdPingRequest:
        sendCommand(cmdPingResponse, data, len);
        break;
        
    case cmdPingResponse:
        if (len == 4) {
            int time = *(int*) data;
            int now = getMiliseconds();
            int diff = (now-time + 1000000) % 1000000;
            
            writeChat("Ping response from %s: %d.%03ds roundtrip time", shortDescription(), diff/1000, diff%1000);
            break;
        } else {
            writeChat("Invalid ping response from %s: not 4 bytes", shortDescription());
        }
        break;
        
    case cmdTextGroup: {
        char group_name[16];
        if (len < (int)sizeof(group_name))
            writeChat("Invalid group message, shorter than %d characters", (int)sizeof(group_name));
        else {
            memcpy(group_name, data, sizeof(group_name)-1);
            group_name[sizeof(group_name)-1] = NUL;

            data += sizeof(group_name)-1;
            len -= sizeof(group_name)-1;
            sane = sanitize(data, len, 1024);
            
            for (int i = sizeof(group_name)-1; i >0; i--)
                if (isspace(group_name[i]))
                    group_name[i] = NUL;

            
            if (config->getOption(opt_chat_paranoia) || !strstr(sane, ~name))
                writeChatText("[%s] %s", shortDescription(), sane);
            else
                writeChatText("%s", sane);
            
            // Now propagate this message
            FOREACH(ChatConnection *, c, chatServerSocket->connections)
                if (c != this && (c->flags & flagServing) && c->group == group_name) {
                    c->sendText(cmdTextGroup, data);
                    if (CDEBUG) writeChat("Propagating to %s", c->shortDescription());
                }
        }
    }
    break;
    
    case cmdStamp:
        if (len < 4)
            writeChat("Got cmdStamp from %s, but it was only %d bytes long", shortDescription(), len);
        else {
            int val = *(int*) data;
            if (val == chatServerSocket->getId()) { // oh my god, we wore the same thing!
                writeChat("I have the same StampID as %s -- regenerating mine", shortDescription());
                chatServerSocket->generateId();
                FOREACH(ChatConnection*,c,chatServerSocket->connections)
                    c->sendStamp();
            }
        }
        
        break;
        
    case cmdRequestConnections:
    case cmdPeekConnections: {
        writeChat("%s asked for a list of our connections", shortDescription());
        Buffer b;
        FOREACH(ChatConnection*,c,chatServerSocket->connections)
            if (!(c->getFlags() & flagPrivate))
                b.printf(",%s,%d", c->getRemoteServerIP(), c->getRemoteServerPort());
        
        if (b.count())
            b.shift(1);
        
        sendCommand(command == cmdRequestConnections ? cmdConnectionList : cmdPeekList, ~b, b.count());
        
    }
    break;
    
    case cmdConnectionList:
    case cmdPeekList: {
        const char *s = sane;
        char *out, *end;
        char ip[64], port_s[64];
        int port;
        unsigned int addr;
        Buffer b;
        int count = 0, new_connections = 0;

        if (CDEBUG) writeChat ("con/peek: %s", sane);
        
        for (;*s;) {
            out = ip; end = ip+sizeof(ip)-1;
            while(*s && *s != ',' && out < end)
                *out++ = *s++;
            if (!*s)
                break;
            *out = NUL; s++;

            out = port_s; end = port_s+sizeof(port_s)-1;
            while(*s && *s != ',' && out < end)
                *out++ = *s++;
            *out = NUL;
            if (*s == ',')
                s++;
            port = atoi(port_s);

            count++;
            
            if (port < 1024 || port > 65000)
                writeChat("%s: discarding %s %d (too high port)", shortDescription(), ip, port);
            else if (( addr = inet_addr(ip)) == (unsigned int) -1)
                writeChat("%s: discarding %s %d (invalid IP address)", shortDescription(), ip, port);
            else {
                if (command == cmdConnectionList) {
                    if (chatServerSocket->findByAddress(ip, port))
                        writeChat("%s:%d already exists, not connecting", ip, port);
                    else {
                        // Don't connect to self :)
                        if (strcmp(chatServerSocket->getIPAddress(), ip)
                            || port != chatServerSocket->getLocalPort()) {
                            chatServerSocket->call(ip,port,zchat);
                            new_connections++;
                        }
                    }
                } else
                    b.printf("%-25s %d\n", ip, port);
            }
        }

        if (command == cmdPeekList)
            writeChat("%s has %d active connections:\n%s", shortDescription(), count, ~b);
        else
            writeChat("%s gave us %d new (total %d) connections", shortDescription(), new_connections, count);
    }

    break;
    
    
    
    case cmdSendCommand:
        if (flags & flagAllowCommands) {
            writeChat("%s executes: %s", shortDescription(), sane);
            interpreter.add(sane);
        } else {
            writeChat("%s tries to execute: %s", shortDescription(), sane);
            sendMessage("You are not allowed to execute commands here");
        }
        break;
        
        
    case cmdTextEverybody:
        if (!(flags & flagIgnored)) {
            if (config->getOption(opt_chat_paranoia) || !strstr(sane, ~name))
                writeChatText("[%s] %s", shortDescription(), sane);
            else
                writeChatText("%s", sane);
        }
        
        // Now propagate this message
        FOREACH(ChatConnection *, c, chatServerSocket->connections)
            if (c != this && (c->flags & flagServing)) {
                c->sendText(cmdTextEverybody, data);
                if (CDEBUG) writeChat("Propagating to %s", c->shortDescription());
            }
        
        break;
        
    case cmdSnoop:
        if (flags & flagSnooping) {
            flags &= ~flagSnooping;
            writeChat("%s stopped snooping you", shortDescription());
        }
        else if (!(flags & flagAllowSnooping)) {
            sendMessage("Snoop request denied.");
            writeChat("%s wanted to snoop you", shortDescription());
        } else
            writeChat("%s is now snooping you.", shortDescription());
        break;
        
    case cmdSnoopData:
        if (flags & flagSnooping) {
            // convert colors and print
        } else {
            writeChat("Snooping data received from %s but snooping is not set", shortDescription());
        }
        break;
        
        
    case cmdStatus:
        if (len < 1)
            writeChat("%s: status packet not 1 byte", shortDescription());
        else {
            int new_status = data[0];
            if (new_status < 0 || new_status > 3)
                writeChat("%s: invalid status %d", shortDescription(), new_status);
            else {
                person_status = (PersonStatus) new_status;
                if (person_status == afk)
                    writeChat("%s is now AFK", shortDescription());
            }
            inactive_since = current_time;
        }
        break;
        
    case cmdVersion:
        if(CDEBUG) writeChat("%s is using %s", shortDescription(), sane);
        version = sane;
        break;
        
    case cmdEmailAddress:
        writeChat("%s's email address is %s", shortDescription(), sane);
        email = sane;
        break;
        
    case cmdPGPKey:
        if (CDEBUG) writeChat("Received %s's PGP key", shortDescription());
        delete pgpKey;
        pgpKey = new byte[len];
        memcpy(pgpKey, data, len);
        break;
        
    case cmdRequestPGPKey:
        sendMessage("Sorry, dirt does not support PGP yet");
        break;
        
        // IGNORE
    case cmdIcon:
        ;
        break;
        
    case cmdNameChange:
        if (strlen(sane) < 64) {
            writeChat("Name change: %s is now %s", shortDescription(), sane);
            name = sane;
        } else
            writeChat("Name change ignored for %s: %s is longer than 64 characters", shortDescription(), sane);
        break;

    case cmdFileStart:
        sendCommand(cmdFileDeny, "");
        sendMessage("dirt does not support file transfer yet");
        break;
        
        
    default:
        writeChat("Received unknown command %d (%d bytes of data) from %s", command, len, shortDescription());
    }
}

void ChatConnection::sendAll() {
    sendVersion();
    if (config->getStringOption(opt_chat_icon).len())
        sendIcon(config->getStringOption(opt_chat_icon));
    sendStatus();
    sendStamp();
    sendEmail();
}

// initial connection request
void ChatConnection::requestConnection() {
    char buf[MAX_MUD_BUF];
    if (protocol == zchat) {
        snprintf(buf, sizeof(buf), "ZCHAT:%s\t%s\n%s%-5u\n%s",
                 ~CHAT_NAME,
                 "0",
                 ~chatServerSocket->ip,
                 chatServerSocket->getLocalPort(),
                 "");
        writeText(buf);
    } else {
        snprintf(buf, sizeof(buf), "CHAT:%s\n%s%05d", ~CHAT_NAME,
                 ~chatServerSocket->ip, chatServerSocket->getLocalPort());
        writeText(buf);
    }
}

bool ChatConnection::acceptConnection() {
    if (state != awaiting_confirmation)
        return false;

    state = connected;
    writeChat("Connection from %s accepted", shortDescription());
    
    writeText(Sprintf("YES:%s\n", ~CHAT_NAME));
    // Send some more information
    sendAll();
    
    return true;
}

bool ChatConnection::rejectConnection() {
    if (state != awaiting_confirmation)
        return false;
    
    if (CDEBUG) writeChat("Connection from %s rejected", shortDescription());
    writeText("NO\n");
    delete this;
    return true;
}

void ChatConnection::sendMessage(const char *s) {
    sendCommand(cmdMessage,s);
}

// Send text. if using zChat protoco, prefix with stamp
void ChatConnection::sendText(int cmd, const char *arg) {
    if (protocol == mudmaster)
        sendCommand(cmd, arg);
    else {
        char buf[strlen(arg)+5];
        *(int*)buf = chatServerSocket->getId();
        memcpy(buf+4, arg, strlen(arg));
        sendCommand(cmd, buf, strlen(arg)+4);
    }
}

void ChatConnection::sendTextPersonal(const char *arg, bool emote) {
    char buf[INPUT_SIZE];
    if (emote)
        snprintf(buf, sizeof(buf), "\n%s %s.\n", ~CHAT_NAME, arg);
    else
        snprintf(buf, sizeof(buf), "\n%s chats to you: '%s'\n", ~CHAT_NAME, arg);
    
    sendText(cmdTextPersonal, buf);

    const char *colored = sanitize(arg, strlen(arg), 1024);
    if (emote)
        writeChatText("[emote to %s] %s %s%c%c.", ~name, ~CHAT_NAME, colored, SET_COLOR, config->getOption(opt_chat_chatcolor));
    else
        writeChatText("You chat to %s: '%s%c%c'", ~name, colored, SET_COLOR, config->getOption(opt_chat_chatcolor));
    
    if (person_status == afk)
        writeChat("Note that %s has been AFK for %ld minutes.", ~name, (current_time-inactive_since)/60);
}

void ChatConnection::sendTextGroup(const char *group, const char *arg, bool emote) {
    char buf[MAX_INPUT_BUF];
    if (emote)
        snprintf(buf, sizeof(buf), "%-15s\n[emote to %s] '%s %s'\n", group, group, ~CHAT_NAME, arg);
    else
        snprintf(buf, sizeof(buf), "%-15s\n%s chats to '%s': '%s'\n", group, ~CHAT_NAME, group, arg);

    sendText(cmdTextGroup, buf);
    
}

void ChatConnection::sendTextEverybody(const char *arg, bool emote) {
    char buf[INPUT_SIZE];
    if (emote)
        snprintf(buf, sizeof(buf), "[emote to all] %s %s", ~CHAT_NAME, arg);
    else
        snprintf(buf, sizeof(buf), "\n%s chats to everybody: '%s'\n", ~CHAT_NAME, arg);
    sendText(cmdTextEverybody, buf);
}

void ChatConnection::sendVersion() {
    sendCommand(cmdVersion, Sprintf("dirt %s", versionToString(VERSION)));
}

void ChatConnection::sendIcon(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp)
        writeChat("Cannot open icon file %s: %m", filename);
    else {
        char buf[4096];
        int count = fread(buf, sizeof(buf), 1, fp);
        fclose(fp);
        if (count == 4096)
            writeChat("Warning: file is 4096+ bytes, may be too big. The icon should be a 16x16 BMP file");
        sendCommand(cmdIcon, buf, count);
    }
}

void ChatConnection::sendPing() {
    int ms = getMiliseconds();
    sendCommand(cmdPingRequest, (char*)&ms, sizeof(ms));
}

void ChatConnection::sendStatus() {
    char status;
    if (chatServerSocket->is_afk)
        status = afk;
    else
        status = active;
    sendCommand(cmdStatus, &status, 1);
}

void ChatConnection::sendStamp() {
    if (protocol == zchat) {
        int id = chatServerSocket->getId();
        sendCommand(cmdStamp, (char*)&id, sizeof(id));
    }
}

void ChatConnection::sendEmail() {
    if (protocol == zchat && config->getStringOption(opt_chat_email))
        sendCommand(cmdEmailAddress, config->getStringOption(opt_chat_email));
}


// Chat flags
static const struct {
    const char *s;
    int letter;
    int val;
} flag_table[] = {
    { "ignored", 'i',    flagIgnored },
    { "private", 'p',    flagPrivate },
    { "commands",'c',    flagAllowCommands },
    { "snoop",   'n',    flagAllowSnooping },
    { "files",   'f',    flagAllowFiles },
    { "serving", 's',    flagServing },
    { "snooped", 'o',    flagSnoopedBy },  // I am SNOOPED by them
    { "snoop",   'd',    flagSnooping },   // I am SNOOPING them
    { NULL, 0, 0 }
};

static int flagLookup(const char *flag) {
    for (int i = 0; flag_table[i].s; i++)
        if (!strcasecmp(flag, flag_table[i].s))
            return flag_table[i].val;
    return -1;
}

static int flagAbbrevLookup(int flag) {
    for (int i = 0; flag_table[i].s; i++)
        if (flag == flag_table[i].letter)
            return flag_table[i].val;
    return -1;
}

static const char * flagString(int flag, bool abbrev) {
    StaticBuffer buf;
    char *s = buf;
    *s = NUL;
    
    for (int i = 0; flag_table[i].s; i++)
        if (flag & flag_table[i].val) {
            if (abbrev)
                s += sprintf(s, "%c", flag_table[i].letter);
            else
                s += sprintf(s, "%s ", flag_table[i].s);
        } else if (abbrev)
                s += sprintf(s, " ");

    if (!abbrev && s>buf)
        *--s = NUL;

    return buf;
}

const char *adjustFlags(const char *flags, int &flag) {
    char arg[MAX_INPUT_BUF];
    char *s;
    
    do {
        enum { set, clear, toggle } action;
        
        flags = one_argument(flags, arg, true);
        s = arg;
        
        if (s[0]) {
            if (s[0] == '+') {
                action = set; s++;
            } else if (s[0] == '-') {
                action = clear; s++;
            } else
                action = toggle;
            
            int flag_val = flagLookup(s);
            if (flag_val == -1)
                return Sprintf("flag %s does not exist", s);
            if (action == set )
                flag |= flag_val;
            else if (action == clear)
                flag &= ~(flag_val);
            else if (action == toggle)
                flag = flag ^ flag_val;
        }
    } while(arg[0]);
    
    return NULL;
}

ChatWindow::ChatWindow(Window *parent) : Selection(parent, wh_full, wh_half, 0, xy_center) {
    setCount(chatServerSocket->getConnectionCount());
    set_top_message("Currently connected chat sessions");
    set_bottom_message("Toggle: Ignored/Private/Commands/sNoop/Files/Serving/snOoped Also:Del,Enter");
}

void ChatWindow::idle() {
    force_update();
}

void ChatWindow::redraw() {
    setCount(chatServerSocket->getConnectionCount());
    Selection::redraw();
}

const char* ChatWindow::getData(int no) {
    ChatConnection *c = chatServerSocket->findByNumber(no);
    if (!c)
        return "<disconnected>";
    else {
        if (width > 90)
            return c->longDescription(true);
        else
            return c->longDescription(false);
    }
}

bool ChatWindow::keypress(int key) {
    int flag;
    ChatConnection *c = chatServerSocket->findByNumber(getSelection());
    
    if (key == key_alt_h) // display only one chat window at a time
        return true;
    else if ((flag = flagAbbrevLookup(key)) != -1) {
        if (c)
            c->setFlags(c->getFlags() ^ flag);
        return true;
    } else if (key == key_delete || key == key_backspace) {
        if (c)
            c->close("User request");
    } else if (key == keyEnter) {
        if (c)
            inputLine->set(Sprintf("%cchat.to %s ", CMDCHAR, ~c->getName()));
        die();
        return true;
    }
    
    return Selection::keypress(key);
}

