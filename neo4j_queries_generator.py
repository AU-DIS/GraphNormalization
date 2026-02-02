from neo4j import GraphDatabase
import random
from sys import argv
import csv

def get_schema_relationships(tx):
    query = """
    MATCH (a)-[r]->(b)
    RETURN DISTINCT a.`~label` AS startLabel, type(r) AS relType, b.`~label` AS endLabel
    """
    result = tx.run(query)
    return [(record["startLabel"], record["relType"], record["endLabel"]) for record in result if record["startLabel"] != "NODE" and record["endLabel"] != "NODE"]

def get_twohop_schema(tx):
    query = """
    MATCH (a)-[r1]->(b)-[r2]->(c)
    RETURN DISTINCT labels(a)[-1] AS startLabel, type(r1) AS rel1,
                    labels(b)[-1] AS midLabel, type(r2) AS rel2,
                    labels(c)[-1] AS endLabel
    """
    result = tx.run(query)
    return [
        (record["startLabel"], record["rel1"], record["midLabel"], record["rel2"], record["endLabel"])
        for record in result
    ]


def get_labels(tx):
    query = """
            MATCH (n)
            WHERE n.`~label` IS NOT NULL
            RETURN DISTINCT n.`~label` AS labelValues
            """
    result = tx.run(query)
    return [record["labelValues"]  for record in result if record["labelValues"] != "NODE"]

def get_properties_for_label(tx, label):
    query = f"""
    MATCH (n)
    WHERE n.`~label` = "{label}"
    RETURN DISTINCT keys(n) AS props
    """
    records = tx.run(query)
    uniques = set()
    for record in records:
        uniques = uniques.union(set(record["props"]))
    return [
        record if record.isalnum() else f'`{record}`'
        for record in list(uniques)
        if "ID"  != record and "IRI" != record  and "neo4jImportId" != record 
    ]

def get_sample_values(tx, label, prop, limit=5):
    query = f"""
    MATCH (n:NODE)
    WHERE n.{prop} IS NOT NULL AND n.`~label` = "{label}"
    RETURN DISTINCT n.{prop} as value
    LIMIT {limit}
    """

    return [record["value"] for record in tx.run(query) if record["value"] is not None]

TEMPLATES = {
    "filter": """MATCH (n:NODE)
                 WHERE n.{prop} = "{value}" AND n.`~label` = "{label}"
                 RETURN n.IRI""",

    "count": """MATCH (n:NODE)
                WHERE n.{prop} = "{value}" AND n.`~label` = "{label}"
                RETURN count(n)""",

    "local_update": """MATCH (n:NODE)-[:{rel}]->(m:NODE)
                 WHERE n.{prop1} = "{value1}" AND m.{prop2} = "{value2}" AND n.`~label` = "{label1}" AND m.`~label` = "{label2}"
                 SET n.{prop1} = "{new_value}"
                 RETURN count(n.{prop1})""",
    
    "update": """MATCH (n:NODE)
                 WHERE n.{prop} = "{value}" AND n.`~label` = "{label}"
                 SET n.{prop} = "{new_value}" 
                 RETURN count(n.{prop})"""

}
def backtick_if_slash(s: str) -> str:
    return f"`{s}`" if "/" in s else s

def generate_query(session, labels, rels, two_hops, template):
    if not labels or not rels:
        return None

    if template_name in ["filter", "update","count"]:
        label = random.choice(labels)
        props = session.execute_read(get_properties_for_label, label)
        if not props:
            return None
        prop = random.choice(props)
        values = session.execute_read(get_sample_values, label, prop)
        if not values:
            return None
        value = random.choice(values)
        if isinstance(value, list): return None
        value = value.replace('"', '')
        new_value = f"{value}_new"
        return template.format(label=label, prop=prop, value=value, new_value=new_value)

    elif template_name == "local_update":
        rel= random.choice(rels)
        props = session.execute_read(get_properties_for_label, rel[0])
        if not props:
            return None
        prop = random.choice(props)
        props2 = session.execute_read(get_properties_for_label, rel[2])
        if not props2:
            return None
        prop2 = random.choice(props2)
        values = session.execute_read(get_sample_values, rel[0], prop)
        if not values:
            return None
        value = random.choice(values)
        if isinstance(value, list): return None
        value = value.replace('"', '')
        values2 = session.execute_read(get_sample_values, rel[2], prop2)
        if not values2:
            return None
        value2 = random.choice(values2)
        if isinstance(value2, list): return None
        value2 = value2.replace('"', '')
        new_value = f"{value}_new"
        relation = backtick_if_slash(rel[1])
        return template.format(label1=rel[0], label2=rel[2],
                                rel=relation, prop1=prop, prop2=prop2, value1=value, value2=value2, new_value=new_value)

def execute_query(tx, query):
    result = tx.run(query)
    value = result.value()
    summary = result.consume()
    return value, summary.result_available_after

if __name__ == "__main__":

    port = "7100"
    if len(argv) > 3 : port = str(argv[3])

    URI = "bolt://localhost:"+port
    USER = "neo4j"
    PASSWORD = "password"

    driver = GraphDatabase.driver(URI, auth=(USER, PASSWORD))

    nb_queries = 10
    if len(argv) > 1 : nb_queries = int(argv[1])
    with driver.session() as session:
        queries = dict()
        print("Generating queries...")
        labels = session.execute_read(get_labels)
        rels = session.execute_read(get_schema_relationships)
        print("Got important info...")
        for template_name in TEMPLATES.keys():
            print(template_name)
            retries = 0
            template = TEMPLATES[template_name]
            queries[template_name] = set()
            while len(queries[template_name]) < nb_queries and retries <= 100:
                q = generate_query(session, labels, rels, None, template)
                if q:
                    queries[template_name].add(q.strip())
                    retries = 0
                retries += 1

        csv_headers = ["Query_type", "Query_text", "Query_result", "Execution_time(ms)"]
        output = []
        print("Excuting queries...")
        for query_type, queries_set in queries.items():
            print()
            print(query_type)
            for query in queries_set :
                query_output = [query_type, query.replace("\n", " ")]
                query_output.extend(execute_query(session, query))
                output.append(query_output)
        output.sort(key=lambda x: x[0])
        if len(argv) > 2:
            filename = argv[2]
        else : 
            filename = "queries_outputs.csv"

        with open(filename, 'w', newline='') as csvfile:
            csvwriter = csv.writer(csvfile)
            csvwriter.writerow(csv_headers)
            csvwriter.writerows(output)  
        
        print(f"Results saved in {filename}")
