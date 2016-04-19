#include <cctype>
#include <memory>
#include <ostream>
#include <string>

#include <boost/algorithm/string/trim.hpp>
#ifdef LEVEL_LOGGING
#include <boost/log/trivial.hpp>
#endif

#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/XMLException.hpp>
#include <xercesc/util/XMLString.hpp>

#include <ddl_names.h>
#include <ddl_names_filereader.h>
#include <ddl_stnames_migrator.h>

#include "dbunit_xml_ddl_names_filereader.h"

namespace middle {
using namespace std;
using namespace boost::algorithm;
using namespace xercesc;
using namespace normal_ddl;

struct xmlstring : public string {
  xmlstring(const XMLCh* xmlp) {
    char* cp{XMLString::transcode(xmlp)};
    string::operator=(cp);
    XMLString::release(&cp);
  }
};

const string assume_identity_keyword{"MIDDLE!ASSUME_IDENTITY"};

bool
dbunit_ddl_names_filereader_impl::read_ddl_names(const string& filepath, ddl_stname_usedcols& stname_usedcols, ddl_assumed_identities& assumed_idents) {
  try {
    const unique_ptr<XercesDOMParser> parser{new XercesDOMParser};
    parser->setValidationScheme(XercesDOMParser::Val_Never);
    parser->parse(filepath.c_str());
    DOMDocument* doc{parser->getDocument()};
    if (doc) {
      if (doc->getDocumentElement() && xmlstring{doc->getDocumentElement()->getNodeName()} == "dataset") {
        for (DOMNode* node = doc->getDocumentElement()->getFirstChild(); node; node = node->getNextSibling()) {
          if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
            // in a dbunit xml flatfile, a dataset element's name is a stname (schema.table name)
            xmlstring table_name{node->getNodeName()};
            const ddl_stname stname{ddl_stname::parse(table_name, app_stnames_migrator_factory.get_default_schema_name())};
            if (stname.empty()) {
#ifdef LEVEL_LOGGING
              BOOST_LOG_TRIVIAL(debug) << "parsing XML file '" << filepath << "': at XML element attribute of child element of <dataset>: invalid DDL table name: '" << table_name << '\'';
#endif
              throw ddl_names_filereader_parse_error{string{"parsing XML file '"} + filepath + "': at XML element attribute of child element of <dataset>: invalid DDL table name: '" + table_name + '\''};
            }
            stname_usedcols.insert_name(stname);

            // the element's attributes are the names of columns used (defined) by the test; note these
            const DOMNamedNodeMap* attrs{node->getAttributes()};
            if (attrs) {
              for (auto i = 0U; i < attrs->getLength(); ++i)
                stname_usedcols.insert_name_usedcol(stname, ddl_cname{xmlstring{attrs->item(i)->getNodeName()}});
            }
          } else if (node->getNodeType() == DOMNode::COMMENT_NODE) {
            const xmlstring comment{node->getNodeValue()};
            const auto pos = comment.find(assume_identity_keyword);
            if (pos != string::npos && isspace(comment[pos + assume_identity_keyword.length()])) {
              string column_name{comment.substr(pos + assume_identity_keyword.length() + 1)};
              trim(column_name);
              const ddl_stcname stcname{ddl_stcname::parse(column_name, app_stnames_migrator_factory.get_default_schema_name())};
              if (stcname.empty()) {
#ifdef LEVEL_LOGGING
                BOOST_LOG_TRIVIAL(debug) << "parsing XML file '" << filepath << "': at XML comment child of <dataset>: invalid '" + assume_identity_keyword + "' DDL column name: '" << column_name << '\'';
#endif
                throw ddl_names_filereader_parse_error{string{"parsing XML file '"} + filepath + "': at XML comment child of <dataset>: invalid '" + assume_identity_keyword + "' DDL column name: '" + column_name + '\''};
              }
              assumed_idents.insert(stcname);
#ifdef LEVEL_LOGGING
              BOOST_LOG_TRIVIAL(info) << "parsing XML file '" << filepath << "': at XML comment child of <dataset>: found '" + assume_identity_keyword + "' DDL column name: '" << column_name << '\'';
#endif
              if (verbose_os)
                *verbose_os << "parsing XML file '" << filepath << "': at XML comment child of <dataset>: found '" + assume_identity_keyword + "' DDL column name: '" << column_name << '\'' << endl;
            }
          }
        }
      } else {
#ifdef LEVEL_LOGGING
        BOOST_LOG_TRIVIAL(debug) << "parsing XML file '" << filepath << "': no XML root element <dataset>";
#endif
        throw ddl_names_filereader_parse_error{string{"parsing XML file '"} + filepath + "': no XML root element <dataset>"};
      }
    } else {
#ifdef LEVEL_LOGGING
      BOOST_LOG_TRIVIAL(debug) << "parsing XML file '" << filepath << "': no XML document parsed";
#endif
      throw ddl_names_filereader_parse_error{string{"parsing XML file '"} + filepath + "': no XML document parsed"};
    }
  } catch (const XMLException& e) {
#ifdef LEVEL_LOGGING
    BOOST_LOG_TRIVIAL(debug) << "parsing XML file '" << filepath << "' at line " << e.getSrcLine() << ": caught xercesc::XMLException: " << xmlstring{e.getMessage()};
#endif
    throw ddl_names_filereader_parse_error(xmlstring{e.getMessage()});
  }
  return true;  // column use noted here
}
}
