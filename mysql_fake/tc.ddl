CREATE TABLE tc_query
(
    id INT PRIMARY KEY NOT NULL AUTO_INCREMENT,
    schema_name VARCHAR(128) NOT NULL,
    table_name VARCHAR(128) NOT NULL,
    column_name VARCHAR(128) NOT NULL,
    column_pos INT NOT NULL,
    default_val VARCHAR(4000),
    is_nullable VARCHAR(3) NOT NULL,
    data_type VARCHAR(128) NOT NULL,
    char_max_len INT,
    char_oct_len INT,
    num_prec TINYINT,
    num_prec_rad SMALLINT,
    num_scale INT,
    dt_prec SMALLINT,
    charset_name VARCHAR(128),
    is_identity BIT NOT NULL
);
ALTER TABLE tc_query ADD CONSTRAINT uq_tcid_position UNIQUE(schema_name, table_name, column_name, column_pos);