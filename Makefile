DAEMONOBJS=TorrentDaemon.o Main.o ListenSocket.o ConnectionSocket.o \
		TorrentManager.o Torrent.o Parser.o soapC.o soapServer.o
CLIENTOBJS=TestClient.o soapC.o soapClient.o

CXXFLAGS=-pthread -I/usr/include/libtorrent -Wall -Werror -Wno-deprecated-declarations
#CXXFLAGS+=-g -ggdb
DAEMONLDFLAGS=-pthread -lboost_date_time -lboost_filesystem -lboost_thread -ltorrent -lhttp_fetcher -llog4cpp -lgsoap
CLIENTLDFLAGS=-lgsoap

SOAPTARGETS=soapC.cpp soapClient.cpp soapClientLib.cpp soapH.h \
							soapServer.cpp soapServerLib.cpp soapStub.h \
							soapTorrentDaemonObject.h soapTorrentDaemonProxy.h \
							TorrentDaemon.nsmap TorrentDaemon.wsdl \
							torrentdaemon.xsd

all: torrentdaemon testclient

clean:
	rm -f $(DAEMONOBJS) $(CLIENTOBJS) torrentdaemon testclient *.d $(SOAPTARGETS) *.xml

$(SOAPTARGETS): soapTorrent.h
		 soapcpp2 soapTorrent.h
		 
%.o: %.cc
	@echo COMPILE $< $@
	@$(CXX) -c -o $@ $(CXXFLAGS) $<

%.o: %.cpp
	@echo COMPILE $< $@
	@$(CXX) -c -o $@ $(CXXFLAGS) $<

%.d: %.cc $(SOAPTARGETS)
	@echo DEPEND $< $@
	@$(CXX) -MM $(CXXFLAGS) $< | \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' > $@

%.d: %.cpp $(SOAPTARGETS)
	@echo DEPEND $< $@
	@$(CXX) -MM $(CXXFLAGS) $< | \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' > $@

torrentdaemon: $(DAEMONOBJS)
	$(CXX) -o $@ $^ $(DAEMONLDFLAGS)
	
testclient: $(CLIENTOBJS)
	$(CXX) -o $@ $^ $(CLIENTLDFLAGS)
	
ifneq "$(MAKECMDGOALS)" "clean"
-include $(DAEMONOBJS:.o=.d)
-include $(CLIENTOBJS:.o=.d)
endif


