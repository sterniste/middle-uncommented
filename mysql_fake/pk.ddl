CREATE TABLE pk_query
(
    id INT PRIMARY KEY NOT NULL AUTO_INCREMENT,
    schema_name VARCHAR(128) NOT NULL,
    table_name VARCHAR(128) NOT NULL,
    column_name VARCHAR(128) NOT NULL
);
ALTER TABLE pk_query ADD CONSTRAINT uq_tcid UNIQUE(schema_name, table_name, column_name);
