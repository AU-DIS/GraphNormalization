### Property Graph Normalization
Code to the experiments to verify effectivness of the graph normalization method.

To compile the main file please use c++11 or higher.

To run the example for one dataset use:
> ./main \<datasetname> \<normal form> \<export graph in NFs>

E.g. for dbpedia to highest normal form with graph results in between

> ./main dbpedia 5 1

For the datasets please download from the respective sites:<br>
https://github.com/neo4j-graph-examples/northwind<br>
https://offshoreleaks.icij.org/pages/database<br>
https://blog.dblp.org/2022/03/02/dblp-in-rdf/<br>
https://www.dbpedia.org/<br>
https://physionet.org/content/mimiciii/1.4/<br>
https://www.geneontology.org/<br>

The files should be stored as datasets/\<datasetname>/\<downloaded files> <br>

To import the resulting json files into your neo4j database use first:
> CALL apoc.import.json("file:///\<datasetname>_nodes.json") <br>

Then:<br>
> CALL apoc.periodic.iterate(<br>
'CALL apoc.load.json("file:///_edges.json") YIELD value as line',<br>
'MATCH (n:NODE), (m:NODE) WHERE n.neo4jImportId=line.start.id AND m.neo4jImportId=line.end.id CALL apoc.create.relationship(n,line.label,line.properties,m) YIELD rel RETURN rel',<br>
{batchSize:100000, parallel:true})<br>
YIELD batches, total, updateStatistics<br>
RETURN batches, total, updateStatistics<br>

Then one can run queries manually on the databases or automatically generated ones with the python script

> neo4j_queries_generator.py
