CC = gcc
OUT_DIR = out
SPRINT_DIR = sprint1

.PHONY: serveur client clean

serveur:
	@echo "Choisissez la version (v0, v1, v2) avec : make serveur VERSION=version PORT=port NBCLI=nbcli" 
	$(CC) -o $(OUT_DIR)/$(SPRINT_DIR)/$(VERSION)/serveur $(SPRINT_DIR)/$(VERSION)/serveur.c -lpthread 
	./$(OUT_DIR)/$(SPRINT_DIR)/$(VERSION)/serveur $(PORT) $(NBCLI)

client:
	@echo "Choisissez la version (v0, v1, v2) avec : make client VERSION=version ADRESSEIP=adresseip PORT=port ORDRE=ordre"
	$(CC) -o $(OUT_DIR)/$(SPRINT_DIR)/$(VERSION)/client $(SPRINT_DIR)/$(VERSION)/client.c -lpthread 
	./$(OUT_DIR)/$(SPRINT_DIR)/$(VERSION)/client $(ADRESSEIP) $(PORT) $(ORDRE)

clean:
	rm -f $(OUT_DIR)/v0/serveur $(OUT_DIR)/v0/client
	rm -f $(OUT_DIR)/v1/serveur $(OUT_DIR)/v1/client
	rm -f $(OUT_DIR)/v2/serveur $(OUT_DIR)/v2/client

serveur2:
	@echo "Choisissez la version (v0, v1, v2) avec : make serveur VERSION=version PORT=port NBCLI=nbcli" 
	$(CC) -o $(OUT_DIR)/sprint2/$(VERSION)/serveur sprint2/$(VERSION)/serveur.c -lpthread 
	./$(OUT_DIR)/sprint2/$(VERSION)/serveur $(PORT) $(NBCLI)

client2:
	@echo "Choisissez la version (v0, v1, v2) avec : make client VERSION=version ADRESSEIP=adresseip PORT=port ORDRE=ordre"
	$(CC) -o $(OUT_DIR)/sprint2/$(VERSION)/client sprint2/$(VERSION)/client.c -lpthread 
	./$(OUT_DIR)/sprint2/$(VERSION)/client $(ADRESSEIP) $(PORT) $(ORDRE)
