CC = gcc
CFLAGS = -Wall -Wextra -g -Iutils -IServer
CLIENTFLAGS = -lcurl -lcjson
OBJDIR = obj

SERVER_SRCS = Server/main.c Server/connection.c Server/message.c
SERVER_OBJS = $(addprefix $(OBJDIR)/, $(notdir $(SERVER_SRCS:.c=.o)))

CLIENT_SRCS = Client/client.c Client/ai.c
CLIENT_OBJS = $(addprefix $(OBJDIR)/, $(notdir $(CLIENT_SRCS:.c=.o)))

UTILS_SRCS = utils/socketutils.c
UTILS_OBJS = $(addprefix $(OBJDIR)/, $(notdir $(UTILS_SRCS:.c=.o)))

all: server client

server: $(SERVER_OBJS) $(UTILS_OBJS)
	$(CC) $(CFLAGS) -o server $(SERVER_OBJS) $(UTILS_OBJS)

client: $(CLIENT_OBJS) $(UTILS_OBJS)
	$(CC) $(CFLAGS) -o client $(CLIENT_OBJS) $(UTILS_OBJS) $(CLIENTFLAGS)

$(OBJDIR)/%.o: Server/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: Client/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: utils/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f server client
	rm -rf $(OBJDIR)
