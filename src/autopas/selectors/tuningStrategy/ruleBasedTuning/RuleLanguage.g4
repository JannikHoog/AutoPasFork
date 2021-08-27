grammar RuleLanguage;

program
                : statement+ ;

WS
                : [ \r\n\t]+ -> skip
                ;

Container_opt
                : 'VerletClusterLists'
                | 'VerletListsCells'
                | 'LinkedCells'
                | 'DirectSum'
                | 'LinkedCellsReferences'
                | 'VarVerletListsAsBuild'
                | 'VerletClusterCells'
                | 'VerletLists'
                | 'PairwiseVerletLists'
                ;

Traversal_opt
                : 'ds_sequential' | 'lc_c01' | 'lc_c01_combined_SoA' | 'lc_c01_cuda' | 'lc_c04' | 'lc_c04_HCP'
                | 'lc_c04_combined_SoA' | 'lc_c08' | 'lc_c18' | 'lc_sliced' | 'lc_sliced_balanced' | 'lc_sliced_c02'
                | 'vcc_cluster_iteration_cuda' | 'vcl_c01_balanced' | 'vcl_c06' | 'vcl_cluster_iteration' | 'vcl_sliced'
                | 'vcl_sliced_balanced' | 'vcl_sliced_c02' | 'vl_list_iteration' | 'vlc_c01' | 'vlc_c18' | 'vlc_sliced'
                | 'vlc_sliced_balanced' | 'vlc_sliced_c02' | 'vlp_c01' | 'vlp_c18' | 'vlp_sliced'
                | 'vlp_sliced_balanced' | 'vlp_sliced_c02' | 'vvl_as_built'
                ;

Load_estimator_opt
                : 'None'
                ;

Data_layout_opt
                : 'AoS'
                | 'SoA'
                ;

Newton3_opt
                : 'enabled'
                | 'disabled'
                ;

Bool_val
                : 'false'
                | 'true'
                ;

Configuration_property
                : 'container'
                | 'traversal'
                | 'dataLayout'
                | 'newton3'
                | 'loadEstimator'
                | 'cellSizeFactor'
                ;

DIGIT: '0'..'9';

Unsigned_val
                : DIGIT+
                ;

Double_val
                : DIGIT+ '.' DIGIT*
                ;

literal
                : '"' Traversal_opt '"'
                | '"' Container_opt '"'
                | '"' Load_estimator_opt '"'
                | '"' Data_layout_opt '"'
                | '"' Newton3_opt '"'
                | Unsigned_val
                | Double_val
                | '"' Bool_val '"'
                ;


fragment LETTER : [a-zA-Z] ;

Variable_name
                : LETTER (LETTER|DIGIT|'_')*;

define_list
                : 'define_list' Variable_name '=' literal (',' literal)* ';' ;

define
                : 'define' Variable_name '=' literal ';' ;

variable
                : Variable_name
                ;

atom_expr
                : variable | literal
                ;

comp_expr
                : atom_expr (op=('>'|'<') atom_expr)?
                ;

expression
                : comp_expr ('and' comp_expr)?
                ;

property_value
                : Variable_name
                | literal
                ;

configuration_pattern
                : '[' (Configuration_property '=' property_value) (',' Configuration_property '=' property_value)* ']'
                ;

configuration_order
                : configuration_pattern '>=' configuration_pattern ';' ;

statement
                : define_list
                | define
                | if_statement
                | configuration_order
                ;

if_statement
                : 'if' expression ':' statement+ 'endif' ;

