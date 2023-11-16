CC = gcc -W

all: engine/engine.exe gameui/gameui.exe bot/bot.exe
	@ echo "Task completed"

debug: gameui-debug engine-debug
	@ echo "Debug compilation concluded."

# Engine Section
engine/engine.exe: engine/engine.o utils/utils.o
	@ $(CC) $^ -o $@
	@ echo "Engine compiled."

engine.o: engine/engine.c engine/engine.h utils/utils.h
	@ $(CC) -c $< -o $@

engine-debug: engine/engine.c engine/engine.h utils-debug
	@ $(CC) -g -c engine/engine.c -o engine/engine.o
	@ $(CC) -g engine/engine.o utils/utils.o -o engine/engine.out
	@ echo "Engine compiled."

# GameUI Section
gameui/gameui.exe: gameui/gameui.o utils/utils.o
	@ $(CC) $^ -o $@ -lncurses
	@ echo "Programa gameui compilado."

gameui.o: gameui/gameui.c gameui/gameui.h utils/utils.h
	@ $(CC) -c $< -o $@

gameui-debug: gameui/gameui.c gameui/gameui.h utils-debug
	@ $(CC) -g -c gameui/gameui.c -o gameui/gameui.o
	@ $(CC) -g gameui/gameui.o utils/utils.o -o gameui/gameui.out
	@ echo "Programa gameui compilado."

# UTILS Section
utils.o: utils/utils.c utils/utils.h
	@ $(CC) -c $< -o $@

utils-debug: utils/utils.c utils/utils.h
	@ $(CC) -g -c $< -o utils/utils.o

#BOT Section
bot/bot.exe: bot/bot.o
	@ $(CC) $^ -o $@
	@ echo "Engine compiled."

bot.o: bot/bot.c
	@ $(CC) -c $< -o $@


# workaround para fazer com que objetos intermédios (neste casos, os ficheiro objeto dos jogos) não sejam automaticamente apagados
.SECONDARY: 

# SECCAO MANUTENCAO
# limpa o diretorio de ficheiros temporarios e/ou de executaveis

clean: clean-obj clean-exe
	@ echo "Diretorio limpo."

clean-obj:
	@ rm ./*/*.o -f

clean-exe:
	@ rm ./*/*.exe -f

clean-out:
	@ rm ./*/*.out -f

clean-pipes:
	@ echo "Pipes deixados por problemas de execução eliminados."
	#@ rm *_w *_r arb_pipe -f
