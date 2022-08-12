server: ftpS
	@echo "Server Starting.."
	@./ftpS

client: ftpC
	@./ftpC

ftpS: ftpS.c
	@gcc $^ -o $@

ftpC: ftpC.c
	@gcc $^ -o $@

clean:
	@rm -rf ftpS ftpC