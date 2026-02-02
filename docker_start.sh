#northwind example for one container
docker run --name northwind_0 -p7400:7474 -p7600:7687 -d -v $HOME/results:/import -v $HOME/conf:/conf -v northwind_0:/data --env NEO4J_PLUGINS='["apoc","n10s","graph-data-science"]' --env NEO4J_AUTH=neo4j/password neo4j:5.18.1-community
