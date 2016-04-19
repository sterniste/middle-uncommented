# Running Middle

## Overview
Essentially, middle looks for (DbUnit XML flatfile) source files in a number of user supplied source trees, possibly aided by user supplied file path patterns. These files are scanned for references to schema-qualified Microsoft SQL Server table names; an HSQLDB compliant DDL output file representing the same tables is then output to the appropriate location in a user supplied target tree. In "batch mode", a single DDL file is output (to the top of the target tree) for every source tree, otherwise an output file is produced per source file successfully scanned, its location in the target tree "mirroring" that of the source file in the source tree. This migration of XML to DDL is driven by the metadata of user supplied SQL Server database connections. Thus, the metadata (database schema, table, column, and constraint definitions referenced or implied by the XML files scanned) of some set of databases is subjected to translation rules (contained in a user supplied JSON preference file) before being output. For example, a preference rule might require that sized integer datatypes (e.g. "INT(10)") in the table metadata be transformed into plain "INT" in HSQLDB table definition DDL. Another rule might request the rejection (dropping) of columns having datatype "IMAGE". DDL comments documenting the transformations applied are output alongside and/or after the translated DDL in the same output file. Middle's output is always mediated by the set of schema-qualified table names encountered in the sources it scans: it is thus impossible to request DDL for all the tables in the original databases. In one special case, (because middle does not migrate triggers) a user can specify (via an XML comment in a scanned source file) that a particular database column be ouput as an HSQLDB identity (auto-increment) column: otherwise the transformation process is entirely determined by user command-line options and JSON preferences. Note that, although any number of databases may be specified by the user, middle will halt with an error if it finds any tables defined in multiple databases, even if those tables are never referenced from within source XML.

## Preconditions
An ODBC entry must exist registering the information provided by the program options _database_, _username_, and _password_.
These are the ODBC registry name, user name, and password for the SQL Server database whose DDL is to be migrated.
Under Windows refer to the Microsoft ODBC Manager product to learn how to register/query this information;
under Linux, refer to the ODBC configuration text files at __/etc/odbc.ini__ and  __/etc/odbcinst.ini__.

## Program Options and Arguments
Most options and non-option arguments to middle are sufficiently described by the _--help_ option,
sample output of which is given below:

    middle (MIgrate DDL Everywhere); version: 1.0-iw-20150817
    usage: middle [options] target-root source-root [source-root ...]
    Command-line options:
      -h [ --help ]                     this help message
      -c [ --config-file ] arg          configuration file
      -t [ --time ]                     output time taken (successful runs only)
      -v [ --verbose ]                  verbose
      --version                         version
    
    Configuration options:
      -d [ --odbc-datasource ] arg      ODBC DSN:username:password
      -g [ --graft-source-paths ]       graft source paths to target root
      -i [ --include-source-paths ] arg include source filepaths (partial regex)
      -x [ --exclude-source-paths ] arg exclude source filepaths (partial regex)
      -j [ --json-prefs-file ] arg      json preference file
      -b [ --batch-sources ]            batch sources under each source root
      --log-file arg                    log to file
      --log-level arg (=info)           log level: [fatal, error, warn, info, 
    

Whereas a few purely command-line options (like _--help_) can only be given on the command line,
other more useful configuration options can either be given on the command line or supplied in a
property style configuration file, an example of which is shown below:

    odbc-datasource = immoweb-dev:UT_Develop:EjcjAGCy2P3yb8VSA2Gf
    odbc-datasource = iw-auth-dev:UT_Java:dBmVZ2fqrnTbvBLkgQ9p
    include-source-paths = /db/.*\.xml$
    json-prefs-file = prefs.json
    batch-sources = true

In two cases (_--include-source-paths_ and _--exclude-source-paths_), an option may be provided
multiple times: either on the command line, or in the configuration file, or in both places.
For these options user supplied values accumulate, command line options preceding configuration file options.
In all other cases where a configuration option is present both on the command line and in a configuration
file, the command-line option takes precedence.

Except when involving the options _--help_ or _--version_ (in which cases all other options will be ignored),
the command line must always be terminated by at least two non-option file path arguments:
1. a target root path: this is an existing directory where the DLL output trees will be anchored; and
2. at least one source root path: these are (XML flatfile) input tree roots, thus existing directories.

## Option Details

* _verbose_: output selected information (such as counts of preferences read, paths of XML flatfiles scanned,
paths of DDL files written, etc.) on the standard output. This is not affected by logging options.
* _odbc-datasource_: the ODBC DSN (name) followed by username and password, separated from each other by the _:_;
* _graft-source-paths_: the source root paths must be relative to the user's working directory
(i.e. they must not be absolute path names): the DDL files migrated to the target root will appear there
with this prefix path.
* _include-source-path_: a regex pattern used to include the (XML flatfile) input files scanned. When used
in conjunction with the _graft-source-paths_, this will be tested against the prefixed source root path.
A file is selected if its path is partially matched by any such pattern (multiple patterns may be specified,
on the command line as well as in the configuration file: they are then applied in the order provided),
but only if not excluded by any _exclude-source-paths_ option. In the absence of all include patterns, every
regular file encountered in the source roots will be scanned.
* _exclude-source-path_: a regex pattern used to exclude the (XML flatfile) input files scanned. When used
in conjunction with the _graft-source-paths_, this will be tested against the prefixed source path.
A file is excluded if its path is partially matched by any such pattern (multiple patterns may be specified,
on the command line as well as in the configuration file: they are then applied in the order provided).
* _json-prefs-file_: the path of a JSON preference file. See the next section for details about its structure.
* _batch-sources_: for each source path, produce a single DDL output file (named __middle-batch-#.ddl__, where
_#_ is the number of the source root on the command line) in the target root. In the absence of this option,
DDL files are migrated per (XML flatfile) input scanned, with paths mirroring those of the input but
having _.ddl_ as file name extension.
* _log-file_: write a log to the given file.
* _log-level_: log messages at the given severity. Possible values are _fatal_, _error_, _warn_, _info_, _debug_,
_trace_; _info_ being the default value. Note that providing _log-level_ without _log-file_ has no effect.

## Assumed Identity Fields
To partially compensate for the fact that insertion triggers are not migrated, middle allows a user to suggest --- via an XML comment in a source file --- that a particular column be treated as an auto-incremented generated column (an identity column, in HSQLDB terms) on output. Thus the DbUnit XML snippet below:

    <dataset>
     <!-- MIDDLE!ASSUME_IDENTITY test.there.id -->
     <test.there id="1"/>
    </dataset>
 
 might (assuming the existence of a corresponding integer column in the original database) produce DDL like:
 
    CREATE TABLE test.there(id INT NOT NULL GENERATED BY DEFAULT AS IDENTITY(START WITH 1, INCREMENT BY 1), 

## JSON Preference File Structure

The preferences in the JSON preference file come under two main headings: datatype mappings are taken from a JSON array named _datatype-rules_, whereas another JSON array _default-value-rules_ describes default value macros.

### Datatype Rules

Datatype mappings control the translation (or suppression or rejection) of the various possible datatypes or their size parameter(s) in SQL Server column metadata. They are introduced by the top level JSON array:

    "datatype_rules": [ ... ]

There are four main categories (types) of datatypes:

* _unsized_: these are invariably unsized data types, such as _BIT_;
* _char\_length_: these are data types with a single mandatory size (meaning length) parameter, such as _CHAR_ and _VARCHAR_;
* _precision\_scale_: these are numeric data types like _INT_ or _NUMERIC_ with either no size parameter (as in _INT_), a single size parameter (precision as in _FLOAT(53)_), or two size parameters (precision and scale as in _NUMERIC(10,3)_);
* _dt\_precision_: these are date/time data types with either no size parameter (as in _DATE_) or a single size parameter (the precision as in _DATETIME2(9)_);

In order to be appear in DDL output, a datatype must be recognized by the _type\_name_ at the top of a rule and associated with one of these categories (via the field _datatype.type_). It is also possible to specify _rejected_ as the _datatype.type_, in which case such columns will be rejected; they will be missing from the DDL output and an error message inside a DDL comment will be generated. However, since all unrecognized datatypes are rejected, this explicit filtering is only useful for documentation purposes. The rejection of a datatype (due either to the lack of a recognizing rule or to the action of some rejecting rule) simply means that all columns with it are filtered from the DDL output (figuring neither in table nor in constraint definitions); error texts will appear in DDL comments explaining their absence, but neither is the integrity of their enclosing DDL compromised, nor does middle terminate. These are thus not errors in the threatening sense and it may be desirable to ignore some of them.

The following snippets show most of what can be done with datatype rules.

The following rule simply recognizes the _unsized_ category _BIT_ datatype, saving all _BIT_ column metadata from rejection.

    {
      "type_name": "BIT",
      "datatype": {
        "type": "unsized"
      }
    }

The following rule recognizes the _char\_length_ category _NVARCHAR_ datatype, translating it to _VARCHAR_. The metadata size is assigned a new value 255 for sizes less than 1 or greater than 255. Note that default values are relied upon for _range\_min_ (= _MININT_) in the first range and for _range\_max_ (= _MAXINT_) in the second. The default value of _new\_value_ is 0. Order is important amongst sub-rules: only the first matching sub-rule is applied; a size not matched by the range of any sub-rule is output unchanged (though a datatype not matching any rule is rejected). Of course, these two sub-rules might easily be merged into one.
    
    {
      "type_name": "NVARCHAR",
      "datatype": {
        "type": "char_length",
        "map_name": "varchar",
        "map_sizes": [
          {
            "type": "assign",
            "range_max": 0,
            "new_value": 255
          },
          {
            "type": "assign",
            "range_min": 256,
            "new_value": 255
          }
 		]
      }
    }
      
The following rule recognizes the _precision\_scale_ category _INT_ datatype. The _map\_sizes_ structure contains 2 sub-rules. The first causes any metadata precision to be suppressed (to disappear without error) from the DDL output. The second narrows metadata scale values to a range from 0 through 0. Narrowing means that metadata values outside the range are matched: lesser values are assigned the range minimum, greater values the range maximum. Narrowing to a single value of 0 requires explanation. SQL Server disallows precision (or _char\_length_ category length) values of 0 but supplies a default scale value of 0 whenever a precision is defined without a scale. Middle will never output any 0 valued size (length, precision, or scale) so that, according to these sub-rules, the metadata definitions _INT_ and _INT(11,0)_ will both translate to the single output _INT_. The same effect would have been obtained by specifying a _type_ of _scale\_suppress_ (without range) in the second sub-rule, or _narrow_ (with 0 through 0 range) in the first. Or the types _assign_ and _scale\_assign_ might have been used with the appropriate ranges to redefine both precision and scale to 0. All scale sub-rules must follow all precision sub-rules within a _precision\_scale_ category rule.

    {
      "type_name": "INT",
      "datatype": {
        "type": "precision_scale",
        "map_sizes": [
          {
            "type": "suppress"
          },
          {
            "type": "scale_narrow",
            "range_min": 0,
            "range_max": 0
          }
        ]
      }
    }

The following rule recognizes the _dt\_precision_ category _DATETIME2_ datatype, translating it to _DATETIME_ and removing any precision.

    {
      "type_name": "DATETIME2",
      "datatype": {
        "type": "dt_precision",
        "map_name": "datetime",
        "map_sizes": [
          {
            "type": "suppress"
          }
        ]
      }
    }

#### Summary of _map\_sizes_ Sub-rule Types

Types marked * are only possible in the _precision\_scale_ category and apply to the second (scale) size: they must follow all unmarked (precision) types within a rule.

    | type                    | action
    | ----------------------- | ------------------------------------------------------------------------------------------------------ |
    | reject_defined          | reject if length/precision defined                                                                     |
    | narrow                  | (non-range matched) length/precision values below range become min_value, above range become max_value |
    | suppress                | drop length/precision                                                                                |
    | assign                  | reset length/precision to new_value                                                                    |
    | assign_undefined        | (range unused) set length/precision to new_value                                                       |
    | reject_undefined        | (range unused) reject if length/precision undefined                                                    |
    | scale_reject_defined*   | reject if scale defined                                                                                |
    | scale_narrow*           | (non-range matched) scale values below range become min_value, above range become max_value            |
    | scale_suppress*         | drop scale                                                                                           |
    | scale_assign*           | reset scale to new_value                                                                               |
    | scale_assign_undefined* | (range unused) set scale to new_value                                                                  |
    | scale_reject_undefined* | (range unused) reject if scale undefined                                                               |

### Default Value Rules

Default value mappings control the translation of the default value expressions in SQL Server column metadata. They are introduced by the top level JSON array:

    "default_value_rules": [ ... ]

Each default value rule recognizes a macro, i.e. an SQL name possibly followed by a set of parentheses enclosing an argument list. The macro left-hand-side is replaced by its right-hand-side and, depending on the rule, an argument list of a given legnth may be expected and its contents rewritten.

For example, the following snippet recognizes the macro _hello_. This rule will translate the SQL name _hello_ to _there_ when the former appears as a name inside a default value expression migrated my middle. Only SQL names are translated; middle will not translate _hello_ inside SQL strings ('_hello_') or within longer SQL names (_hello1_). Because this rule does not define any argument list, none need be present. No checks on the length or rewriting of the argument list will occur if indeed one is found.

    {
      "macro_lhs": "hello",
      "macro_rhs": "there"
    }
      
In the following example the SQL name _getdate_ is translated to _now_ before a mandatory argument list of length 0, as specified by the _macro\_args_ sub-structure. Now the SQL name _getdate_ found in a default value expression without an argument list (or followed by a non-empty argument list) will cause middle to terminate in error. The same check would be accomplished by an empty _macro\_args_ sub-structure (since _arg\_count_ defaults to 0).

    {
      "macro_lhs": "getdate",
      "macro_rhs": "now",
      "macro_args": {
        "arg_count": 0
      }
    }

