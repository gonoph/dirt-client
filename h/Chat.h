#ifndef __CHAT_H
#define __CHAT_H

#include "Socket.h"
#include "Hook.h"

// MudMaster/ZCHAT chat implementation

enum Protocol {                              // Protocol supported by the remote side
    zchat, mudmaster
};

// Commands
// s = accepts but not useful
// S = accepts and fully implemented
// C = can send out
enum {
    cmdNameChange = 1,                   //  S
    cmdRequestConnections = 2,           // CS
    cmdConnectionList = 3,               // CS
    cmdTextEverybody = 4,                // CS
    cmdTextPersonal = 5,                 // CS
    cmdTextGroup = 6,                    // CS
    cmdMessage = 7,                      // CS
    cmdVersion = 19,                     // CS
    cmdFileStart = 20,                   //  s
    cmdFileDeny = 21,                    //  s
    cmdFileBlockRequest = 22,            //  s
    cmdFileBlock = 23,                   //  s
    cmdFileEnd = 24,                     //  s
    cmdFileCancel = 25,                  //  s
    cmdPingRequest = 26,                 // CS
    cmdPingResponse = 27,                // CS
    cmdPeekConnections = 28,             // CS
    cmdPeekList = 29,                    // CS
    cmdSnoop = 30,                       // CS
    cmdSnoopData = 31,                   //  S
    cmdIcon = 100,                       // Cs
    cmdStatus = 101,                     // CS
    cmdEmailAddress = 102,               // CS
    cmdRequestPGPKey = 103,              //  s
    cmdPGPKey = 104,                     //  s
    cmdSendCommand = 105,                // CS
    cmdStamp = 106                       // CS
};

enum ConnectionFlags {
    flagIgnored = 0x0001,           // Ignore stuff from this connection
    flagPrivate = 0x0002,           // Don't reveal this connection to others
    flagAllowCommands = 0x0004,     // Honor commands
    flagAllowSnooping = 0x0008,     // they may snoop me
    flagAllowFiles =    0x0010,     // They may send me files
    flagServing =       0x0020,     // Forward chats I receive to them
    flagSnoopedBy =     0x0040,     // I am currently snooped by them
    flagSnooping =      0x0080,     // I am currently snooping THEM
    flagRequestPending =0x0100,     // Expect a chatRequest or chatPeekReply from this user
};

// An incoming or outgoing connection, one for each connection
class ChatConnection : public Socket {
public:
    ChatConnection(int _fd = -1, struct sockaddr_in *remote = NULL); // if fd != -1, this is an incoming connection we have already established and state=incoming
    ~ChatConnection();
    
    void connect (const char *hostname, int port, int _protocol); // Start up a connection to there (state=connecting)
    const char *shortDescription() const;          // Short description of this connection
    const char* longDescription(bool verbose);     // Full description
    void close (const char *reason);               // Close connection to this client, with this explanation
    void handleIncoming();                         // starting point for incoming connections
    void idle();
    
    bool acceptConnection();
    bool rejectConnection();
    
    void sendTextPersonal(const char*, bool);
    void sendTextEverybody(const char *arg, bool);
    void sendTextGroup(const char *group, const char *arg, bool emote);
    void sendMessage(const char *);
    void sendIcon(const char *name);
    void sendStatus();
    void sendPing();

    void sendCommand(int command, const char *data, int len= -1);
    
    void setFlags(int _flags) { flags = _flags; }
    int getFlags() const { return flags; };

    // These may be different than the remote address
    const char *getRemoteServerIP() const { return ip; }
    int getRemoteServerPort() const { return remote_port; }

    const String& getGroup() { return group; }
    void setGroup(const char *s) { group = s; }
    
    
    const String& getName() { return name; }
    
private:
    // From Socket
    virtual void connectionEstablished();
    virtual void inputReady();
    virtual void errorEncountered(int);
    
    void requestConnection();
    
    
    void dispatchCommand (int command, const char *data, int len);
    void sendText(int cmd, const char *arg);
    
    void sendAll(); // send a lot of the interesting information
    
    
    void sendVersion();
    void sendStamp();
    void sendEmail();
    
    // small state diagram
    enum {
        invalid,                        // initial state
        connecting,                     // connect() called. ON Connected: write request data, GOTO requesting
        requesting,                     // ON YES GOTO connected. ON NO disconnect and die
        connected,                      // Wait for incoming command
        waiting_command_header,         // Waiting for the full 4 bytes of the zChat command header
        waiting_command_data,           // Waiting for the command data (pending_data more bytes)
        waiting_data,                   // waiting for the 255-terminated data
        incoming_chatname,              // incoming connection established. wait for chatname line -> incoming_address
        incoming_address,               // waiting for address information -> incoming_security OR end
        incoming_security,              // waiting for security info (if client == zchat) -> END
        // END: autoaccept ? write YES, GOTO connected
        // no autoaccept ? inform user, GOTO awaiting_confirmation
        
        awaiting_confirmation           // ON user accept: write YES, GOTO connected. ON user deny: write NO, disconnect
    } state;
    
    Protocol protocol;

    String name;                        // chat name
    String id;                          // Not set by zchat 1.x apparently
    String version;                     // Client version
    String group;                       // Chat group
    String email;                       // Email address
    byte *pgpKey;
    int flags;
    
    enum PersonStatus {
        none, active, inactive, afk
    } person_status;
    
    time_t inactive_since;                // When did we receive the inactive/AFK packet
    time_t connect_time;                // When did we establish the connection
    time_t timeout;                     // When will we stop bothering to connect
    
    String ip;                          // remote's ip and port it listens on
    int remote_port;
    
    bool accepted;                      // Did we originally accept this connection or did we connect there?
    
    int pending_command;                // Command of which something we are receiving
    int data_length;
    Buffer command_data;
    
    int command_header_received;
    char command_header[4];
    
};

// unique, contains ChatConnections
class ChatServerSocket : public Socket {
public:
    // Called from #chat.command
    void call (const char *target, int port, Protocol protocol); // create new ChatConnection to there
    
    static void create(int base_port); // bind to the port etc.
    void idle();
    void handleUserCommand(const char *name, const char *arg);
    ChatConnection* findConnection(const char *name);

    ChatConnection* findByAddress(const char *ip, int port);

    ChatConnection* findByNumber(int no) { return connections[no]; }
    int getConnectionCount() const { return connections.count(); }
    
    int getId()                    { return id; }
    void generateId();
    void computeIPAddress();
    
    const char * findIPAddress();

    const char* getIPAddress() { return ip; }

    ~ChatServerSocket();
    void handleSnooping(const char*data, int len);
    
private:
    virtual void connectionAccepted(int, struct sockaddr_in*); // new incoming connection
    
    ChatServerSocket(); // bind to port
    List<ChatConnection*> connections;
    
    friend ChatConnection;
    int id; // Stamp ID
    bool is_afk;
    int snoop_count;
    
    String ip;
};

extern ChatServerSocket *chatServerSocket;

// The chat window
class ChatWindow : public Selection {
public:
    ChatWindow(Window *parent);
private:
    const char *getData(int count);
    virtual bool keypress(int key);
    virtual void idle();
    virtual void redraw();
};


#endif
