CREATE TABLE ck_query
(
    id INT PRIMARY KEY NOT NULL AUTO_INCREMENT,
    schema_name VARCHAR(128) NOT NULL,
    table_name VARCHAR(128) NOT NULL,
    column_name VARCHAR(128) NOT NULL,
    constraint_name VARCHAR(128) NOT NULL,
    check_clause VARCHAR(4000) NOT NULL
);
ALTER TABLE ck_query ADD CONSTRAINT uq_tcid_constraint UNIQUE(schema_name, table_name, column_name, constraint_name);