#include "languagenodes_p.h"
#include "live/visuallog.h"
#include "live/stacktrace.h"

#include <queue>
#include <map>
#include <vector>
#include <algorithm>
#include <set>

namespace lv{ namespace el{

std::string indent(int i){
    std::string res;
    res.append(4 * i, ' ');
    return res;
}

BaseNode::BaseNode(const TSNode &node, const std::string &typeString)
    : m_parent(nullptr)
    , m_node(node)
    , m_typeString(typeString)
{}

BaseNode::~BaseNode(){
    for ( auto it = m_children.begin(); it != m_children.end(); ++it ){
        delete *it;
    }
}

BaseNode *BaseNode::visit(LanguageParser::AST *ast){
    TSTree* tree = reinterpret_cast<TSTree*>(ast);
    TSNode root_node = ts_tree_root_node(tree);

    ProgramNode* node = new ProgramNode(root_node);

    uint32_t count = ts_node_child_count(root_node);

    for ( uint32_t i = 0; i < count; ++i ){
        TSNode child = ts_node_child(root_node, i);
        visit(node, child);
    }

    return node;
}

bool BaseNode::checkIdentifierDeclared(const std::string& source, BaseNode *node, std::string id){
    if (id == "this" || id == "parent" || id == "Element" || id == "Container" || id == "import" || id == "vlog" || id == "console" )
        return true;

    BaseNode* prog = nullptr;
    while (node){
        JsBlockNode* hasIds = dynamic_cast<JsBlockNode*>(node);

        if (!node->parent()){
            prog = node;
        }

        if (hasIds){
            for (auto it = hasIds->m_declarations.begin(); it != hasIds->m_declarations.end(); ++it){
                std::string declaredId = slice(source, *it);
                if (declaredId == id){
                    return true;
                }
            }
        }

        node = node->parent();
    }

    return false;
}

const std::vector<BaseNode *>& BaseNode::children() const{
    return m_children;
}

int BaseNode::startByte() const{
    return ts_node_start_byte(m_node);
}

int BaseNode::endByte() const{
    return ts_node_end_byte(m_node);
}

std::string BaseNode::rangeString() const{
    return std::string("[") + std::to_string(startByte()) + ", " + std::to_string(endByte()) + "]";
}

void BaseNode::convertToJs(const std::string &source, std::vector<ElementsInsertion *> &sections, int indentValue){
    for ( BaseNode* node : m_children ){
        node->convertToJs(source, sections, indentValue);
    }
}

void BaseNode::collectImports(const std::string &source, std::vector<IdentifierNode*> &imp){
    for ( BaseNode* node : m_children ){
        node->collectImports(source, imp);
    }
}

std::string BaseNode::slice(const std::string &source, uint32_t start, uint32_t end){
    return source.substr(start, end - start);
}

std::string BaseNode::slice(const std::string &source, BaseNode *node){
    return slice(source, node->startByte(), node->endByte());
}

JsBlockNode* BaseNode::addToDeclarations(BaseNode *parent, IdentifierNode *idNode){
    BaseNode* p = parent;
    while (p){
        JsBlockNode* jsbn = dynamic_cast<JsBlockNode*>(p);
        if (jsbn){
            jsbn->m_declarations.push_back(idNode);
            return jsbn;
            break;
        }
        p = p->parent();
    }
    return nullptr;
}

JsBlockNode *BaseNode::addUsedIdentifier(BaseNode *parent, IdentifierNode *idNode){
    BaseNode* p = parent;
    while (p){
        JsBlockNode* jsbn = dynamic_cast<JsBlockNode*>(p);
        if (jsbn){
            jsbn->m_usedIdentifiers.push_back(idNode);
            return jsbn;
        }
        p = p->parent();
    }

    return nullptr;
}

void BaseNode::addChild(BaseNode *child){
    m_children.push_back(child);
    child->setParent(this);
}

std::string BaseNode::astString() const{
    char* str = ts_node_string(m_node);
    std::string result(str);
    free(str);
    return result;
}

std::string BaseNode::toString(int indent) const{
    std::string result;
    if ( indent > 0 )
        result.assign(indent * 2, ' ');

    result += m_typeString + " [" +
            std::to_string(ts_node_start_byte(m_node)) + ", " +
            std::to_string(ts_node_end_byte(m_node)) + "]\n";

    for (BaseNode* child : m_children){
        result += child->toString(indent < 0 ? indent : indent + 1);
    }

    return result;
}

void BaseNode::visit(BaseNode *parent, const TSNode &node){
    if ( strcmp(ts_node_type(node), "import_statement") == 0 ){
        visitImport(parent, node);
    } else if ( strcmp(ts_node_type(node), "identifier") == 0 ){
        visitIdentifier(parent, node);
    } else if ( strcmp(ts_node_type(node), "constructor_definition") == 0 ){
        visitConstructorDefinition(parent, node);
    } else if ( strcmp(ts_node_type(node), "property_identifier") == 0 ){
        visitIdentifier(parent, node);
    } else if ( strcmp(ts_node_type(node), "this") == 0 ){
        visitIdentifier(parent, node);
    } else if ( strcmp(ts_node_type(node), "import_path") == 0 ){
        visitImportPath(parent, node);
    } else if ( strcmp(ts_node_type(node), "component_declaration") == 0 ){
        visitComponentDeclaration(parent, node);
    } else if ( strcmp(ts_node_type(node), "new_component_expression") == 0 ){
        visitNewComponentExpression(parent, node);
    } else if ( strcmp(ts_node_type(node), "arrow_function") == 0 ){
        visitArrowFunction(parent, node);
    } else if ( strcmp(ts_node_type(node), "component_body") == 0 ){
        visitComponentBody(parent, node);
    } else if ( strcmp(ts_node_type(node), "class_declaration") == 0 ){
        visitClassDeclaration(parent, node);
    } else if ( strcmp(ts_node_type(node), "property_declaration") == 0 ){
        visitPropertyDeclaration(parent, node);
    } else if ( strcmp(ts_node_type(node), "member_expression") == 0 ){
        visitMemberExpression(parent, node);
    } else if ( strcmp(ts_node_type(node), "subscript_expression") == 0 ){
        visitSubscriptExpression(parent, node);
    } else if ( strcmp(ts_node_type(node), "identifier_property_assignment") == 0 ){
        visitIdentifierAssignment(parent, node);
    } else if ( strcmp(ts_node_type(node), "property_assignment") == 0 ){
        visitPropertyAssignment(parent, node);
    } else if ( strcmp(ts_node_type(node), "event_declaration") == 0 ){
        visitEventDeclaration(parent, node);
    } else if ( strcmp(ts_node_type(node), "listener_declaration") == 0 ){
        visitListenerDeclaration(parent, node);
    } else if ( strcmp(ts_node_type(node), "method_definition") == 0 ){
        visitMethodDefinition(parent, node);
    } else if ( strcmp(ts_node_type(node), "typed_function_declaration") == 0 ){
        visitTypedFunctionDeclaration(parent, node);
    } else if ( strcmp(ts_node_type(node), "function_declaration") == 0 ){
        visitFunctionDeclaration(parent, node);
    } else if ( strcmp(ts_node_type(node), "number") == 0 ){
        visitNumber(parent, node);
    } else if ( strcmp(ts_node_type(node), "expression_statement") == 0 ){
        visitExpressionStatement(parent, node);
    } else if ( strcmp(ts_node_type(node), "call_expression") == 0 ){
        visitCallExpression(parent, node);
    } else if ( strcmp(ts_node_type(node), "new_tagged_component_expression") == 0 ){
        visitNewTaggedComponentExpression(parent, node);
    } else if ( strcmp(ts_node_type(node), "tagged_type_string") == 0 ){
        visitTaggedString(parent, node);
    } else if ( strcmp(ts_node_type(node), "variable_declaration") == 0 ){
        visitVariableDeclaration(parent, node);
    } else if ( strcmp(ts_node_type(node), "new_expression") == 0 ){
        visitNewExpression(parent, node);
    } else if ( strcmp(ts_node_type(node), "return_statement") == 0 ){
        visitReturnStatement(parent, node);
    } else if ( strcmp(ts_node_type(node), "object") == 0 ){
        visitObject(parent, node);
    } else if ( strcmp(ts_node_type(node), "ERROR") == 0 ){
        SyntaxException se = CREATE_EXCEPTION(SyntaxException, "Syntax error has happened", Exception::toCode("~LanguageNodes"));
        BaseNode* p = parent;
        while (p && p->typeString() == "Program") p = p->parent();
        se.setParseLocation(ts_node_start_point(node).row, ts_node_start_point(node).column, ts_node_start_byte(node), p ? static_cast<ProgramNode*>(p)->m_fileName : "");
        throw se;
    } else {
        visitChildren(parent, node);
    }
}

void BaseNode::visitChildren(BaseNode *parent, const TSNode &node){
    uint32_t count = ts_node_child_count(node);
    for ( uint32_t i = 0; i < count; ++i ){
        TSNode child = ts_node_child(node, i);
        visit(parent, child);
    }
}

void BaseNode::visitImport(BaseNode *parent, const TSNode &node){
    ImportNode* importNode = new ImportNode(node);
    parent->addChild(importNode);
    uint32_t count = ts_node_child_count(node);
    for ( uint32_t i = 0; i < count; ++i ){
        TSNode child = ts_node_child(node, i);
        if ( strcmp(ts_node_type(child), "import_as" ) == 0 ){
            TSNode aliasChild = ts_node_child(child, 1);
            IdentifierNode* in = new IdentifierNode(aliasChild);
            importNode->m_importAs = in;
            importNode->addChild(in);
        } else if ( strcmp(ts_node_type(child), ".") == 0){
            importNode->m_isRelative = true;
        } else if ( strcmp(ts_node_type(child), "import_path") == 0 ){
            visitImportPath(importNode, child);
        }
    }
    visitChildren(importNode, node);
}

void BaseNode::visitIdentifier(BaseNode *parent, const TSNode &node){
    IdentifierNode* identifierNode = new IdentifierNode(node);
    parent->addChild(identifierNode);
    visitChildren(identifierNode, node);
}

void BaseNode::visitImportPath(BaseNode *parent, const TSNode &node){
    ImportPathNode* ipnode = new ImportPathNode(node);
    uint32_t count = ts_node_child_count(node);
    for ( uint32_t i = 0; i < count; ++i ){
        TSNode child = ts_node_child(node, i);
        if ( strcmp(ts_node_type(child), "identifier") == 0 ){
            IdentifierNode* in = new IdentifierNode(child);
            ipnode->m_segments.push_back(in);
            ipnode->addChild(in);
        }
    }
    parent->addChild(ipnode);
}

void BaseNode::visitComponentDeclaration(BaseNode *parent, const TSNode &node){
    ComponentDeclarationNode* enode = new ComponentDeclarationNode(node);
    parent->addChild(enode);
    if (parent->typeString() == "PropertyDeclaration"){
        static_cast<PropertyDeclarationNode*>(parent)->m_componentDeclaration = enode;
    }

    uint32_t count = ts_node_child_count(node);
    for ( uint32_t i = 0; i < count; ++i ){
        TSNode child = ts_node_child(node, i);
        if ( strcmp(ts_node_type(child), "identifier") == 0 ){
            enode->m_name = new IdentifierNode(child);
            enode->addChild(enode->m_name);
            addToDeclarations(parent, enode->m_name);
        } else if ( strcmp(ts_node_type(child), "component_heritage") == 0 ){
            uint32_t heritageCount = ts_node_child_count(child);
            for ( uint32_t j = 0; j < heritageCount; ++j ){
                TSNode heritageSegment = ts_node_child(child, j);
                if ( strcmp(ts_node_type(heritageSegment), "identifier" ) == 0 ){
                    IdentifierNode* heritageSegmentNode = new IdentifierNode(heritageSegment);
                    enode->m_heritage.push_back(heritageSegmentNode);
                    enode->addChild(heritageSegmentNode);
                }
            }
            if ( enode->m_heritage.size() > 0 ){
                addUsedIdentifier(enode, enode->m_heritage[0]);
            }
        } else if ( strcmp(ts_node_type(child), "component_body") == 0 ){
            enode->m_body = new ComponentBodyNode(child);
            enode->addChild(enode->m_body);
            visitChildren(enode->m_body, child);
        } else if ( strcmp(ts_node_type(child), "component_identifier") == 0 ){
            if ( ts_node_named_child_count(child) > 0 ){
                TSNode idChild = ts_node_child(child, 1);
                enode->m_id = new IdentifierNode(idChild);
                enode->addChild(enode->m_id);
                addToDeclarations(parent, enode->m_id);
            }
        }
    }
}

void BaseNode::visitComponentBody(BaseNode *parent, const TSNode &node){
    ComponentBodyNode* enode = new ComponentBodyNode(node);
    parent->addChild(enode);
    visitChildren(enode, node);
}

void BaseNode::visitNewComponentExpression(BaseNode *parent, const TSNode &node){
    NewComponentExpressionNode* enode = nullptr;

    if (parent->typeString() == "ExpressionStatement"
        && parent->parent()
        && parent->parent()->typeString() == "Program")
    {
        enode = new RootNewComponentExpressionNode(node);

        ProgramNode* pn = dynamic_cast<ProgramNode*>(parent->parent());
        pn->m_exports.push_back(enode);
    } else {
        BaseNode* p = parent;
        while (p && dynamic_cast<JsBlockNode*>(p) == nullptr)
        {
            p = p->parent();
        }
        if (p && p->typeString() == "JSScope"){
            enode = new RootNewComponentExpressionNode(node);
        } else {
            enode = new NewComponentExpressionNode(node);
        }
    }

    parent->addChild(enode);
    uint32_t count = ts_node_child_count(node);
    for ( uint32_t i = 0; i < count; ++i ){
        TSNode child = ts_node_child(node, i);
        if ( strcmp(ts_node_type(child), "identifier") == 0 ){
            auto iden = new IdentifierNode(child);
            enode->m_name.push_back(iden);
            enode->addChild(iden);
            if ( enode->m_name.size() == 1 )
                addUsedIdentifier(enode, iden);
        } else if ( strcmp(ts_node_type(child), "component_body") == 0 ){
            enode->m_body = new ComponentBodyNode(child);
            enode->addChild(enode->m_body);
            visitChildren(enode->m_body, child);
        } else if ( strcmp(ts_node_type(child), "arguments") == 0 ){
            enode->m_arguments = new ArgumentsNode(child);
            enode->addChild(enode->m_arguments);
            visitChildren(enode->m_arguments, child);
            for ( auto child : enode->m_arguments->children() ){
                if (child->typeString() == "Identifier"){
                    addUsedIdentifier(enode->m_arguments, child->as<IdentifierNode>());
                }
            }

        } else if ( strcmp(ts_node_type(child), "component_identifier") == 0 ){
            TSNode id = ts_node_child(child, 1);
            if (strcmp(ts_node_type(id), "identifier") != 0) continue;
            enode->m_id = new IdentifierNode(id);
            enode->addChild(enode->m_id);
        } else if ( strcmp(ts_node_type(child), "component_instance") == 0 ){
            TSNode id = ts_node_child(child, 1);
            if (strcmp(ts_node_type(id), "identifier") != 0)
                continue;
            enode->m_instance = new IdentifierNode(id);
            enode->addChild(enode->m_instance);

        }
    }

    // for id components
    BaseNode* p = parent;

    if ( enode->m_id ){
        while (p && enode->typeString() != "RootNewComponentExpression"){
            if (p->typeString() == "ComponentBody"){
                auto body = p->as<ComponentBodyNode>();
                if (body->parent()->typeString() == "ComponentDeclaration")
                {
                    auto decl = body->parent()->as<ComponentDeclarationNode>();
                    decl->pushToIdComponents(enode);
                    addToDeclarations(body, enode->m_id);
                    break;
                }
            } else if (p->typeString() == "RootNewComponentExpression"){
                auto rnce = p->as<RootNewComponentExpressionNode>();
                rnce->m_idComponents.push_back(enode);
                addToDeclarations(rnce, enode->m_id);
                break;
            }

            p = p->parent();
        }
    }


    // for default property components
    p = parent;
    if (p->typeString() == "ComponentBody")
    {
        auto body = p->as<ComponentBodyNode>();
        if (body->parent()->typeString() == "ComponentDeclaration")
        {
            auto decl = body->parent()->as<ComponentDeclarationNode>();
            decl->pushToDefault(enode);

        }
        else if (body->parent()->typeString() == "NewComponentExpression" || body->parent()->typeString() == "RootNewComponentExpression")
        {
            auto expr = body->parent()->as<NewComponentExpressionNode>();
            expr->pushToDefault(enode);
        }

    }

}

void BaseNode::visitPropertyDeclaration(BaseNode *parent, const TSNode &node){
    PropertyDeclarationNode* enode = new PropertyDeclarationNode(node);
    parent->addChild(enode);
    uint32_t count = ts_node_child_count(node);
    for ( uint32_t i = 0; i < count; ++i ){
        TSNode child = ts_node_child(node, i);
        if ( strcmp(ts_node_type(child), "identifier") == 0 ){
            enode->m_type = new IdentifierNode(child);
            enode->addChild(enode->m_type);
        } else if ( strcmp(ts_node_type(child), "property_identifier") == 0 ){
            enode->m_name = new IdentifierNode(child);
            enode->addChild(enode->m_name);
            // moveToDeclarations(parent, enode->m_name);
        } else if ( strcmp(ts_node_type(child), "expression_statement") == 0 ){
            enode->m_expression = new BindableExpressionNode(child);
            enode->addChild(enode->m_expression);
            visitChildren(enode->m_expression, child);
        } else if (strcmp(ts_node_type(child),"statement_block") == 0) {
            enode->m_statementBlock = new JsBlockNode(child);
            enode->addChild(enode->m_statementBlock);
            visitChildren(enode->m_statementBlock, child);
        } else if (strcmp(ts_node_type(child),"component_declaration") == 0) {
            visitComponentDeclaration(enode, child);

        }
    }

    if (parent->parent() && parent->parent()->typeString() == "ComponentDeclaration"){
        parent->parent()->as<ComponentDeclarationNode>()->pushToProperties(enode);
    }

    if (parent->parent() && (parent->parent()->typeString() == "NewComponentExpression" ||
        parent->parent()->typeString() == "RootNewComponentExpression"))
    {
        parent->parent()->as<NewComponentExpressionNode>()->pushToProperties(enode);
    }
}

void BaseNode::visitBindableExpression(BaseNode *parent, const TSNode &node){
    BindableExpressionNode* enode = new BindableExpressionNode(node);
    parent->addChild(enode);
    visitChildren(enode, node);
}

void BaseNode::visitMemberExpression(BaseNode *parent, const TSNode &node){
    MemberExpressionNode* enode = new MemberExpressionNode(node);
    parent->addChild(enode);
    visitChildren(enode, node);
    if ( enode->children().size() > 0 ){

        auto child = enode->children()[0];
        if ( child->typeString() == "Identifier" )
            addUsedIdentifier(enode, child->as<IdentifierNode>())->typeString();

        BaseNode* p = parent;
        while (p)
        {
            if (p->typeString() == "MemberExpression") break;
            if (p->typeString() == "FunctionDeclaration") break;
            if (p->typeString() == "ClassDeclaration") break;
            if (p->typeString() == "ArrowFunction") break;
            if (p->typeString() == "CallExpression") break;
            if (p->typeString() == "PropertyDeclaration")
            {
                auto child = enode->children()[0];
                if (child->typeString() != "MemberExpression" && child->typeString() != "Identifier")
                    break;
                p->as<PropertyDeclarationNode>()->pushToBindings(enode);
                break;
            }
            if (p->typeString() == "PropertyAssignment")
            {
                auto child = enode->children()[0];
                if (child->typeString() != "MemberExpression" && child->typeString() != "Identifier")
                    break;
                p->as<PropertyAssignmentNode>()->m_bindings.push_back(enode);
                break;
            }
            p = p->parent();
        }
    }


}

void BaseNode::visitSubscriptExpression(BaseNode *parent, const TSNode &node){
    SubscriptExpressionNode* enode = new SubscriptExpressionNode(node);
    parent->addChild(enode);
    visitChildren(enode, node);
}

void BaseNode::visitPropertyAssignment(BaseNode *parent, const TSNode &node){
    PropertyAssignmentNode* enode = new PropertyAssignmentNode(node);
    parent->addChild(enode);
    uint32_t count = ts_node_child_count(node);
    if (count >= 1)
    {
        TSNode lhs = ts_node_child(node, 0);
        if (strcmp(ts_node_type(lhs), "property_assignment_lhs") == 0)
        {
            for (uint32_t i = 0; i < ts_node_child_count(lhs); i+=2)
            {
                auto id = new IdentifierNode(ts_node_child(lhs, i));
                enode->m_property.push_back(id);
                enode->addChild(id);
            }
        }
    }
    if (count >= 3)
    {
        TSNode rhs = ts_node_child(node, 2);
        if ( strcmp(ts_node_type(rhs), "expression_statement") == 0 ){
            enode->m_expression = new BindableExpressionNode(rhs);
            enode->addChild(enode->m_expression);
            visitChildren(enode->m_expression, rhs);
        } else if (strcmp(ts_node_type(rhs),"statement_block") == 0) {
            enode->m_statementBlock = new JsBlockNode(rhs);
            enode->addChild(enode->m_statementBlock);
            visitChildren(enode->m_statementBlock, rhs);
        }
    }

    if (parent->parent() && parent->parent()->typeString() == "ComponentDeclaration")
    {
        parent->parent()->as<ComponentDeclarationNode>()->pushToAssignments(enode);
    }

    if (parent->parent() && (parent->parent()->typeString() == "NewComponentExpression" || parent->parent()->typeString() == "RootNewComponentExpression"))
    {
        parent->parent()->as<NewComponentExpressionNode>()->pushToAssignments(enode);
    }
}

void BaseNode::visitIdentifierAssignment(BaseNode *parent, const TSNode &node){
    BaseNode* componentParent = parent ? parent->parent() : nullptr;
    if ( !componentParent )
        return;

    TSNode idChild = ts_node_child(node, 2);

    if ( componentParent->typeString() == "ComponentDeclaration" ){
        ComponentDeclarationNode* cdn = componentParent->as<ComponentDeclarationNode>();
        cdn->m_id = new IdentifierNode(idChild);
        // provide id to outside scope
        addToDeclarations(componentParent, cdn->m_id);
    } else if ( componentParent->typeString() == "NewComponentExpression" || componentParent->typeString() == "RootNewComponentExpression"){
        NewComponentExpressionNode* ncen = componentParent->as<NewComponentExpressionNode>();
        ncen->m_id = new IdentifierNode(idChild);
        // provide id to outside scope
        addToDeclarations(componentParent, ncen->m_id);
    }
}

void BaseNode::visitEventDeclaration(BaseNode *parent, const TSNode &node){
    EventDeclarationNode* enode = new EventDeclarationNode(node);
    parent->addChild(enode);

    uint32_t count = ts_node_child_count(node);
    for ( uint32_t i = 0; i < count; ++i ){
        TSNode child = ts_node_child(node, i);
        if ( strcmp(ts_node_type(child), "property_identifier") == 0 ){
            enode->m_name = new IdentifierNode(child);
        } else if ( strcmp(ts_node_type(child), "formal_type_parameters") == 0 ){
            enode->m_parameters = new ParameterListNode(child);

            uint32_t paramterCount = ts_node_named_child_count(child);

            if ( paramterCount % 2 == 0 ){
                for ( uint32_t j = 0; j < paramterCount; j += 2 ){
                    TSNode paramType = ts_node_named_child(child, j);
                    TSNode paramName = ts_node_named_child(child, j + 1);
                    enode->m_parameters->m_parameters.push_back(
                        std::make_pair(new IdentifierNode(paramType), new IdentifierNode(paramName))
                    );
                }
            }
        }
    }

    if (parent->parent() && parent->parent()->typeString() == "ComponentDeclaration")
    {
        parent->parent()->as<ComponentDeclarationNode>()->m_events.push_back(enode);
    }
}

void BaseNode::visitListenerDeclaration(BaseNode *parent, const TSNode &node){
    ListenerDeclarationNode* enode = new ListenerDeclarationNode(node);
    parent->addChild(enode);

    uint32_t count = ts_node_child_count(node);
    for ( uint32_t i = 0; i < count; ++i ){
        TSNode child = ts_node_child(node, i);
        if ( strcmp(ts_node_type(child), "property_identifier") == 0 ){
            enode->m_name = new IdentifierNode(child);
        } else if ( strcmp(ts_node_type(child), "formal_parameters") == 0 ){
            enode->m_parameters = new ParameterListNode(child);
            uint32_t paramterCount = ts_node_named_child_count(child);

            for ( uint32_t j = 0; j < paramterCount; ++j ){
                TSNode paramName = ts_node_named_child(child, j);
                IdentifierNode* paramNameId = new IdentifierNode(paramName);
                enode->m_parameters->m_parameters.push_back(std::make_pair(nullptr, paramNameId));
            }
        } else if ( strcmp(ts_node_type(child), "statement_block") == 0 ){
            enode->m_body = new JsBlockNode(child);
            enode->addChild(enode->m_body);
            visitChildren(enode->m_body, child);
        }
    }

    if (parent->parent() && parent->parent()->typeString() == "ComponentDeclaration"){
        parent->parent()->as<ComponentDeclarationNode>()->m_listeners.push_back(enode);
    }


    if (enode->m_body){
        for (size_t i = 0; i != enode->m_parameters->m_parameters.size(); ++i){
            auto pair = enode->m_parameters->m_parameters[i];
            addToDeclarations(enode->m_body, pair.second);
        }
    }
}

void BaseNode::visitMethodDefinition(BaseNode *parent, const TSNode &node){
    MethodDefinitionNode* enode = new MethodDefinitionNode(node);
    parent->addChild(enode);

    uint32_t count = ts_node_child_count(node);
    for ( uint32_t i = 0; i < count; ++i ){
        TSNode child = ts_node_child(node, i);
        if ( strcmp(ts_node_type(child), "property_identifier") == 0 ){
            enode->m_name = new IdentifierNode(child);
        } else if ( strcmp(ts_node_type(child), "formal_parameters") == 0 ){
            enode->m_parameters = new ParameterListNode(child);
            uint32_t paramterCount = ts_node_named_child_count(child);

            for ( uint32_t j = 0; j < paramterCount; ++j ){
                TSNode paramName = ts_node_named_child(child, j);
                enode->m_parameters->m_parameters.push_back(
                    std::make_pair(nullptr, new IdentifierNode(paramName))
                );
            }
        } else if ( strcmp(ts_node_type(child), "statement_block") == 0 ){
            enode->m_body = new JsBlockNode(child);
            enode->addChild(enode->m_body);
            visitChildren(enode->m_body, child);
        }
    }
}

void BaseNode::visitTypedFunctionDeclaration(BaseNode *parent, const TSNode &node){
    TypedFunctionDeclarationNode* enode = new TypedFunctionDeclarationNode(node);
    parent->addChild(enode);

    uint32_t count = ts_node_child_count(node);
    for ( uint32_t i = 0; i < count; ++i ){
        TSNode child = ts_node_child(node, i);
        if ( strcmp(ts_node_type(child), "property_identifier") == 0 ){
            enode->m_name = new IdentifierNode(child);
        } else if ( strcmp(ts_node_type(child), "formal_type_parameters") == 0 ){
            enode->m_parameters = new ParameterListNode(child);
            uint32_t paramterCount = ts_node_child_count(child);

            for (uint32_t pc = 0; pc < paramterCount; ++pc){
                TSNode ftpc = ts_node_child(child, pc);
                if (strcmp(ts_node_type(ftpc), "formal_type_parameter") == 0){
                    TSNode paramType = ts_node_child(ftpc, 0);
                    TSNode paramName = ts_node_child(ftpc, 1);

                    auto typeNode = new IdentifierNode(paramType);
                    auto nameNode = new IdentifierNode(paramName);

                    enode->m_parameters->m_parameters.push_back(
                        std::make_pair(typeNode, nameNode)
                    );
                }
            }
        } else if ( strcmp(ts_node_type(child), "statement_block") == 0 ){
            enode->m_body = new JsBlockNode(child);
            enode->addChild(enode->m_body);
            visitChildren(enode->m_body, child);
        }
    }

    if (enode->m_body){
        for (size_t i = 0; i != enode->m_parameters->m_parameters.size(); ++i){
            auto pair = enode->m_parameters->m_parameters[i];
            addToDeclarations(enode->m_body, pair.second);
        }
    }
}

void BaseNode::visitPublicFieldDeclaration(BaseNode *parent, const TSNode &node){
    PublicFieldDeclarationNode* enode = new PublicFieldDeclarationNode(node);
    parent->addChild(enode);
    visitChildren(enode, node);
}

void BaseNode::visitJsScope(BaseNode *parent, const TSNode &node){
    JsBlockNode* enode = new JsBlockNode(node);
    parent->addChild(enode);
    visitChildren(enode, node);
}

void BaseNode::visitNumber(BaseNode *parent, const TSNode &node)
{
    NumberNode* nnode = new NumberNode(node);
    parent->addChild(nnode);
}

void BaseNode::visitConstructorDefinition(BaseNode *parent, const TSNode &node)
{
    ConstructorDefinitionNode* cdnode = new ConstructorDefinitionNode(node);
    parent->addChild(cdnode);
    // visitChildren(cdnode, node);

    uint32_t count = ts_node_child_count(node);
    for ( uint32_t i = 0; i < count; ++i ){
        TSNode child = ts_node_child(node, i);
        if ( strcmp(ts_node_type(child), "statement_block") == 0 ){
            cdnode->m_block = new JsBlockNode(child);
            cdnode->addChild(cdnode->m_block);
            visitChildren(cdnode->m_block, child);
            // enode->m_name = new IdentifierNode(child);
        } else if ( strcmp(ts_node_type(child), "formal_parameters") == 0 ){
            cdnode->m_formalParameters = new FormalParametersNode(child);
            cdnode->addChild(cdnode->m_formalParameters);
            visitChildren(cdnode->m_formalParameters,child);
        } else
            visit(cdnode, child);
    }

    if (parent->typeString() == "ComponentBody"){
        parent->as<ComponentBodyNode>()->setConstructor(cdnode);
    }
}

void BaseNode::visitExpressionStatement(BaseNode *parent, const TSNode &node)
{
    ExpressionStatementNode* esnode = new ExpressionStatementNode(node);
    parent->addChild(esnode);
    visitChildren(esnode, node);
}

void BaseNode::visitCallExpression(BaseNode *parent, const TSNode &node)
{
    CallExpressionNode* enode = new CallExpressionNode(node);
    parent->addChild(enode);

    uint32_t count = ts_node_child_count(node);
    for ( uint32_t i = 0; i < count; ++i ){
        TSNode child = ts_node_child(node, i);
        if ( strcmp(ts_node_type(child), "arguments") == 0 ){
            enode->m_arguments = new ArgumentsNode(child);
            enode->addChild(enode->m_arguments);
            visitChildren(enode->m_arguments, child);

            for ( BaseNode* childNode : enode->m_arguments->children() ){
                if ( childNode->typeString() == "Identifier" ){
                    addUsedIdentifier(enode, childNode->as<IdentifierNode>());
                }
            }

        } else if ( strcmp(ts_node_type(child), "super") == 0 ) {
            enode->isSuper = true;
            if (parent->parent() && parent->parent()->parent()
                    && parent->parent()->parent()->typeString() == "ConstructorDefinition")
            {
                parent->parent()->parent()->as<ConstructorDefinitionNode>()->setSuperCall(enode);
            }
        } else {
            visit(enode, child);
        }
    }
}

void BaseNode::visitNewTaggedComponentExpression(BaseNode *parent, const TSNode &node)
{
    NewTaggedComponentExpressionNode* tagnode = new NewTaggedComponentExpressionNode(node);
    parent->addChild(tagnode);
    visitChildren(tagnode, node);

    // for default property components
    BaseNode* p = parent;
    if (p->typeString() == "ComponentBody")
    {
        auto body = p->as<ComponentBodyNode>();
        if (body->parent()->typeString() == "ComponentDeclaration")
        {
            auto decl = body->parent()->as<ComponentDeclarationNode>();
            decl->pushToDefault(tagnode);
        }
        else if (body->parent()->typeString() == "NewComponentExpression" || body->parent()->typeString() == "RootNewComponentExpression")
        {
            auto expr = body->parent()->as<NewComponentExpressionNode>();
            expr->pushToDefault(tagnode);
        }
    }


    for (auto child: tagnode->children()){
        if (child->typeString() == "Identifier") {
            addUsedIdentifier(tagnode, child->as<IdentifierNode>());
        }
    }
}

void BaseNode::visitTaggedString(BaseNode *parent, const TSNode &node)
{
    TaggedStringNode* tsnode = new TaggedStringNode(node);
    parent->addChild(tsnode);
}

void BaseNode::visitFunctionDeclaration(BaseNode *parent, const TSNode &node)
{
    FunctionDeclarationNode* enode = new FunctionDeclarationNode(node);
    parent->addChild(enode);

    uint32_t count = ts_node_child_count(node);
    for ( uint32_t i = 0; i < count; ++i ){
        TSNode child = ts_node_child(node, i);
        if ( strcmp(ts_node_type(child), "property_identifier") == 0 ){
            enode->m_name = new IdentifierNode(child);
        } else if ( strcmp(ts_node_type(child), "formal_parameters") == 0 ){
            uint32_t paramterCount = ts_node_child_count(child);

            for (uint32_t pc = 0; pc < paramterCount; ++pc){
                TSNode ftpc = ts_node_child(child, pc);
                if (strcmp(ts_node_type(ftpc), "identifier") == 0){
                    auto nameNode = new IdentifierNode(ftpc);
                    enode->m_parameters.push_back(nameNode);
                    enode->addChild(nameNode);
                }
            }
        } else if ( strcmp(ts_node_type(child), "statement_block") == 0 ){
            enode->m_body = new JsBlockNode(child);
            enode->addChild(enode->m_body);
            visitChildren(enode->m_body, child);
        }
    }

    if (enode->m_body){
        for (size_t i = 0; i != enode->m_parameters.size(); ++i){
            addToDeclarations(enode->m_body, enode->m_parameters[i]);
        }
    }
}

void BaseNode::visitClassDeclaration(BaseNode *parent, const TSNode &node)
{
    ClassDeclarationNode* fnode = new ClassDeclarationNode(node);
    parent->addChild(fnode);
    visitChildren(fnode, node);

    for (auto it = fnode->children().begin(); it != fnode->children().end(); ++it)
        if ((*it)->typeString() == "Identifier")
        {
            // this must be the id!
            IdentifierNode* id = static_cast<IdentifierNode*>(*it);
            addToDeclarations(parent, id);
        }
}

void BaseNode::visitVariableDeclaration(BaseNode *parent, const TSNode &node)
{
    VariableDeclarationNode* vdn = new VariableDeclarationNode(node);
    parent->addChild(vdn);

    uint32_t count = ts_node_child_count(node);
    for ( uint32_t i = 0; i < count; ++i ){
        TSNode child = ts_node_child(node, i);
        if ( strcmp(ts_node_type(child), "variable_declarator") == 0 ){
            VariableDeclaratorNode* decl = new VariableDeclaratorNode(child);
            decl->setParent(vdn);
            vdn->m_declarators.push_back(decl);

            uint32_t subcount = ts_node_child_count(child);
            bool equalsFound = false;

            for ( uint32_t k = 0; k < subcount; ++k ){
                TSNode smaller_child = ts_node_child(child, k);
                if ( strcmp(ts_node_type(smaller_child), "identifier") == 0 ){
                    if ( equalsFound ){
                        auto inode = new IdentifierNode(smaller_child);
                        decl->addChild(inode);
                        addUsedIdentifier(decl, inode);
                    } else { // add to scope declarations
                        IdentifierNode* id = new IdentifierNode(smaller_child);
                        decl->addChild(id);
                        addToDeclarations(decl, id);
                    }
                } else if ( strcmp(ts_node_type(smaller_child), "=") == 0 ){
                    equalsFound = true;
                } else {
                    visit(decl, smaller_child);
                }
            }
        } else if ( strcmp(ts_node_type(child), ";") == 0 ) {
            vdn->m_hasSemicolon = true;
        } else {
            visit(vdn, child);
        }
    }

}

void BaseNode::visitNewExpression(BaseNode *parent, const TSNode &node)
{
    NewExpressionNode* nenode = new NewExpressionNode(node);
    parent->addChild(nenode);
    nenode->setParent(parent);
    uint32_t count = ts_node_child_count(node);
    for ( uint32_t i = 0; i < count; ++i ){
        TSNode child = ts_node_child(node, i);
        if ( strcmp(ts_node_type(child), "identifier") == 0 ){
            auto inode = new IdentifierNode(child);
            inode->setParent(nenode);
            nenode->addChild(inode);
            addUsedIdentifier(nenode, inode);
        }
    }
    visitChildren(nenode, node);
}

void BaseNode::visitReturnStatement(BaseNode *parent, const TSNode &node)
{
    ReturnStatementNode* rsnode = new ReturnStatementNode(node);
    parent->addChild(rsnode);
    visitChildren(rsnode, node);
}

void BaseNode::visitArrowFunction(BaseNode *parent, const TSNode &node)
{
    ArrowFunctionNode* enode = new ArrowFunctionNode(node);
    parent->addChild(enode);

    uint32_t count = ts_node_child_count(node);
    for ( uint32_t i = 0; i < count; ++i ){
        TSNode child = ts_node_child(node, i);
        if ( strcmp(ts_node_type(child), "property_identifier") == 0 ){
            enode->m_name = new IdentifierNode(child);
        } else if ( strcmp(ts_node_type(child), "formal_parameters") == 0 ){
            uint32_t paramterCount = ts_node_child_count(child);

            for (uint32_t pc = 0; pc < paramterCount; ++pc){
                TSNode ftpc = ts_node_child(child, pc);
                if (strcmp(ts_node_type(ftpc), "identifier") == 0){
                    auto nameNode = new IdentifierNode(ftpc);
                    enode->m_parameters.push_back(nameNode);
                    enode->addChild(nameNode);
                }
            }
        } else if ( strcmp(ts_node_type(child), "statement_block") == 0 ){
            enode->m_body = new JsBlockNode(child);
            enode->addChild(enode->m_body);
            visitChildren(enode->m_body, child);
        }
    }

    if (enode->m_body){
        for (size_t i = 0; i != enode->m_parameters.size(); ++i){
            addToDeclarations(enode->m_body, enode->m_parameters[i]);
        }
    }
}

void BaseNode::visitObject(BaseNode *parent, const TSNode &node)
{
    ObjectNode* onode = new ObjectNode(node);
    parent->addChild(onode);
    visitChildren(onode, node);
}

std::string ImportNode::toString(int indent) const{
    std::string result;
    if ( indent > 0 )
        result.assign(indent * 2, ' ');

    std::string importPath;
    if ( m_importPath ){
        for( IdentifierNode* node : m_importPath->segments() ){
            importPath += node->rangeString();
        }
    }

    std::string importAs = m_importAs ? "(as " + m_importAs->rangeString() + ")" : "";

    result += std::string("Import (" + importPath + ")") + importAs + rangeString() + "\n";

    return result;
}

std::string ImportNode::path(const std::string& source) const{
    std::string importSegments;

    if (m_isRelative)
        importSegments = ".";

    auto segments = m_importPath->segments();
    for (size_t i = 0; i < segments.size(); ++i){
        auto node = segments[i];
        if (i != 0)
            importSegments += ".";
        importSegments += slice(source, node);
    }

    return importSegments;
}

std::string ImportNode::as(const std::string &source) const{
    if ( m_importAs )
        return slice(source, m_importAs);
    return "";
}

void ImportNode::convertToJs(const std::string &source, std::vector<ElementsInsertion *> &fragments, int /*indentValue*/){
    if ( !m_importPath )
        return;

    ElementsInsertion* compose = new ElementsInsertion;
    compose->from = startByte();
    compose->to   = endByte();

    std::string importSegments = path(source);

    if ( m_importAs ){
        *compose << ("var " + as(source) + " = imports.requireAs(\'" + importSegments + "\')\n");
    } else {
        *compose << ("imports.require(\'" + importSegments + "\')\n");
    }

    fragments.push_back(compose);
}

void ImportNode::addChild(BaseNode *child)
{
    if (child->typeString() == "ImportPath")
    {
        m_importPath = child->as<ImportPathNode>();
        child->setParent(this);
        return;
    }
    BaseNode::addChild(child);
}

void ProgramNode::collectImportTypes(const std::string &source){
    if ( m_importTypesCollected )
        return;

    std::vector<IdentifierNode*> identifiers;
    collectImports(source, identifiers);

    for ( auto identifier : identifiers ){

        std::string idName = slice(source, identifier);

        bool isNamespaceIdentifier = false;

        // check whether identifier is part of an import namespace
        for ( ImportNode* in : m_imports ){
            if ( in->hasNamespace() ){
                if ( in->as(source) == idName ){
                    isNamespaceIdentifier = true;
                    BaseNode* parent = identifier->parent();
                    if ( parent->typeString() == "ComponentDeclaration" ){
                        auto parentCast = parent->as<ComponentDeclarationNode>();
                        if ( parentCast->heritage().size() > 1 && parentCast->heritage()[0] == identifier ){
                            ProgramNode::ImportType impt;
                            impt.importNamespace = idName;
                            impt.name = slice(source, parentCast->heritage()[1]);
                            addImportType(impt);
                        }
                    } else if ( parent->typeString() == "MemberExpression" ){
                        auto parentCast = parent->as<MemberExpressionNode>();
                        if ( parentCast->children().size() > 1 && parentCast->children()[0] == identifier ){
                            ProgramNode::ImportType impt;
                            impt.importNamespace = idName;
                            impt.name = slice(source, parentCast->children()[1]);
                            addImportType(impt);
                        }
                    } else if ( parent->typeString() == "NewComponentExpression" || parent->typeString() == "RootNewComponentExpression" ){
                        auto parentCast = parent->as<NewComponentExpressionNode>();
                        if ( parentCast->name().size() > 1 && parentCast->name()[0] == identifier ){
                            ProgramNode::ImportType impt;
                            impt.importNamespace = idName;
                            impt.name = slice(source, parentCast->name()[1]);
                            addImportType(impt);
                        }
                    }
                }
            }
        }

        if ( !isNamespaceIdentifier ){
            ProgramNode::ImportType impt;
            impt.name = idName;
            addImportType(impt);
        }
    }

    m_importTypesCollected = true;
}

void ProgramNode::resolveImport(const std::string &as, const std::string &name, const std::string &path){
    auto asIt = m_importTypes.find(as);
    if ( asIt != m_importTypes.end() ){
        auto nameIt = asIt->second.find(name);
        if ( nameIt != asIt->second.end() ){
            nameIt->second.resolvedPath = path;
        }
    }
}

std::string ProgramNode::importTypesString() const{
    std::string result;
    for ( auto it = m_importTypes.begin(); it != m_importTypes.end(); ++it ){
        if ( !result.empty() )
            result += '\n';
        result += "Imports as '" + it->first + "':";

        for ( auto impIt = it->second.begin(); impIt != it->second.end(); ++ impIt ){
            result += " " + impIt->second.name;
        }
    }
    return result;
}

void ProgramNode::addChild(BaseNode *child){
    if ( child->typeString() == "Import" ){
        m_imports.push_back(child->as<ImportNode>());
        child->setParent(this);
        return;
    } else if ( child->typeString() == "ComponentDeclaration" ){
        m_exports.push_back(child);
        BaseNode::addChild(child);
    } else if ( child->typeString() == "RootNewComponentExpression" ){
        m_exports.push_back(child);
        BaseNode::addChild(child);
    } else
        BaseNode::addChild(child);
}

std::string ProgramNode::toString(int indent) const{
    std::string result;
    if ( indent > 0 )
        result.assign(indent * 2, ' ');

    result += std::string("Program [") +
            std::to_string(ts_node_start_byte(current())) + ", " +
            std::to_string(ts_node_end_byte(current())) + "]\n";

    std::string indentStr;
    if ( indent > 0 )
        indentStr.assign(indent * 2, ' ');

    result += indentStr + " /Imports\n";
    for (BaseNode* child : m_imports){
        result += child->toString(indent < 0 ? indent : indent + 1);
    }

    result += indentStr + " /Component Exports\n";
    for (BaseNode* child : m_exports){
        result += child->toString(indent < 0 ? indent : indent + 1);
    }

    result += indentStr + " /Children\n";
    for (BaseNode* child : children()){
        result += child->toString(indent < 0 ? indent : indent + 1);
    }

    return result;
}

void ProgramNode::convertToJs(const std::string &source, std::vector<ElementsInsertion *> &fragments, int indentValue){
    ElementsInsertion* importsCompose = new ElementsInsertion;
    importsCompose->from = 0;
    int to = 0;
    for ( ImportNode* node: m_imports ){
        if ( node->endByte() > to )
            to = node->endByte();
    }
    importsCompose->to = to;

    for ( auto it = m_importTypes.begin(); it != m_importTypes.end(); ++it ){

        if ( it->first.empty() ){
            for ( auto impIt = it->second.begin(); impIt != it->second.end(); ++ impIt ){
                std::string impPath = impIt->second.resolvedPath.empty() ? "__UNRESOLVED__" : impIt->second.resolvedPath;
                *importsCompose << ("import {" + impIt->second.name + "} from '" + impPath + "'\n");
            }
        } else {
            std::string moduleWrap = "let " + it->first + " = {";
            for ( auto impIt = it->second.begin(); impIt != it->second.end(); ++ impIt ){
                std::string impPath = impIt->second.resolvedPath.empty() ? "__UNRESOLVED__" : impIt->second.resolvedPath;
                std::string impKey = "__" + it->first + "__" + impIt->second.name;
                std::string impName = impIt->second.name;

                if ( impIt != it->second.begin() )
                    moduleWrap += ", ";
                moduleWrap += impIt->second.name + ":" + impKey;

                *importsCompose << ("import {" + impName + " as " + impKey + "} from '" + impPath + "'\n");
            }
            moduleWrap += "}\n";
            *importsCompose << moduleWrap;
        }
    }

    *importsCompose << "\n";

    fragments.push_back(importsCompose);

    size_t offset = fragments.size();

    for ( BaseNode* child: m_exports )
        child->convertToJs(source, fragments, indentValue);

//    BaseNode::convertToJs(source, fragments, indentValue);

    if (m_idComponents.empty())
        return;
    
    auto iter = fragments.begin() + offset;
    ElementsInsertion* compose = new ElementsInsertion;
    if (offset == fragments.size()) {
        compose->from = compose->to = static_cast<int>(source.length());
    } else {
        compose->from = compose->to = (*iter)->from;
    }

    fragments.insert(iter, compose);
}

void ProgramNode::collectImports(const std::string &source, std::vector<IdentifierNode *> &identifiers){
    for ( BaseNode* child: m_exports ){
        child->collectImports(source, identifiers);
    }
}

void ProgramNode::addImportType(const ProgramNode::ImportType &t){
    auto asIt = m_importTypes.find(t.importNamespace);
    if ( asIt == m_importTypes.end() ){
        std::map<std::string, ProgramNode::ImportType> imports;
        imports[t.name] = t;
        m_importTypes[t.importNamespace] = imports;
    } else {
        asIt->second[t.name] = t;
    }
}

ComponentDeclarationNode::ComponentDeclarationNode(const TSNode &node)
    : JsBlockNode(node, "ComponentDeclaration")
    , m_name(nullptr)
    , m_id(nullptr)
    , m_body(nullptr)
{
}

std::string ComponentDeclarationNode::toString(int indent) const{
    std::string result;
    if ( indent > 0 )
        result.assign(indent * 2, ' ');

    std::string name = "(name " + (m_name ? m_name->rangeString() : "") + ")";

    std::string inheritance = "";
    if ( m_heritage.size() > 0 ){
        inheritance = "(inherits ";
        for( IdentifierNode* node :m_heritage ){
            inheritance += node->rangeString();
        }
        inheritance += ")";
    }

    std::string identifier = "";
    if ( m_id ){
        identifier = "(id " + m_id->rangeString() + ")";
    }

    result += "ComponentDeclaration " + name + identifier + inheritance + rangeString() + "\n";
    if ( m_body ){
        result += m_body->toString(indent >= 0 ? indent + 1 : indent);
    }

    return result;
}

void ComponentDeclarationNode::convertToJs(const std::string &source, std::vector<ElementsInsertion *> &fragments, int indentValue){
    ElementsInsertion* compose = new ElementsInsertion;
    compose->from = startByte();
    compose->to   = endByte();

    std::string componentName = name(source);

    *compose << indent(indentValue);

    if (parent() && parent()->typeString() == "Program")
    {
        *compose << "\nexport ";
    }

    std::string heritage = "";
    if ( m_heritage.size() > 0 ){
        heritage = slice(source, m_heritage[0]);
        for ( size_t i = 1; i < m_heritage.size(); ++i ){
            heritage += "." + slice(source, m_heritage[i]);
        }
    }

    *compose << ("class " + componentName + " extends " + (heritage.empty() ? "Element" : heritage) + "{\n\n");

    // handle constructor
    if (m_body->constructor()){
        *compose << indent(indentValue + 1) <<  "constructor";
        if (m_body->constructor()->m_formalParameters)
        {
            *compose << slice(source, m_body->constructor()->m_formalParameters);
        } else *compose << "()";
        *compose << "{\n";
        *compose << indent(indentValue + 2);
        std::string block;
        if (!m_body->m_constructor->m_superCall)
        {
            *compose << "super()\n";
            block = slice(source, m_body->constructor()->m_block);
            block = block.substr(1, block.length() - 2);
        } else {
            *compose << slice(source, m_body->m_constructor->m_superCall);
            block = slice(source, m_body->m_constructor->m_superCall->endByte(), m_body->constructor()->m_block->endByte() - 1);
        }

        *compose << "\n" << indent(indentValue + 2) << "this.__initialize()\n";
        *compose << block;


        *compose << "\n" << indent(indentValue + 1) << "}\n\n";

    } else {
        *compose << indent(indentValue + 1) << "constructor(){\n"
                 << indent(indentValue + 2) << "super()\n"
                 << indent(indentValue + 2) << "this.__initialize()\n"
                 << indent(indentValue + 1) << "}\n\n";
    }

    *compose << indent(indentValue + 1) << "__initialize(){\n";

    if (m_id || !m_idComponents.empty())
        *compose << indent(indentValue + 1) << "this.ids = {}\n\n";

    if (m_id){
        *compose << indent(indentValue + 1) << "var " << slice(source, m_id) << " = this\n";
        *compose << indent(indentValue + 1) << "this.ids[\"" << slice(source, m_id) << "\"] = " << slice(source, m_id) << "\n\n";
    }

    for (size_t i = 0; i < m_idComponents.size();++i)
    {
        *compose << indent(indentValue + 1) << "var " << slice(source, m_idComponents[i]->id()) << " = new " << m_idComponents[i]->initializerName(source);
        if (m_idComponents[i]->arguments())
            *compose << slice(source, m_idComponents[i]->arguments()) << "\n";
        else
            *compose << "()\n";
        *compose << indent(indentValue + 1) << "this.ids[\"" << slice(source, m_idComponents[i]->id()) << "\"] = " << slice(source, m_idComponents[i]->id()) << "\n\n";
    }

    for (size_t i = 0; i < m_idComponents.size();++i)
    {
        std::string id = slice(source, m_idComponents[i]->id());
        auto properties = m_idComponents[i]->properties();

        for (uint32_t idx = 0; idx < properties.size(); ++idx)
        {
            *compose << indent(indentValue + 1) << "Element.addProperty(" << id << ", '" << slice(source, properties[idx]->name())
                     << "', { type: \"" << slice(source, properties[idx]->type()) << "\", notify: \""
                     << slice(source, properties[idx]->name()) << "Changed\" })\n";
        }
    }

    for (size_t i=0; i < m_properties.size(); ++i)
    {
        *compose << indent(indentValue + 1) << "Element.addProperty(this, '" << slice(source, m_properties[i]->name())
                 << "', { type: \"" << slice(source, m_properties[i]->type()) << "\", notify: \""
                 << slice(source, m_properties[i]->name()) << "Changed\" })\n";
    }

    for (size_t i = 0; i < m_events.size(); ++i)
    {
        std::string paramList = "";
        if ( m_events[i]->parameterList() ){
            ParameterListNode* pdn = m_events[i]->parameterList()->as<ParameterListNode>();
            for ( auto it = pdn->parameters().begin(); it != pdn->parameters().end(); ++it ){
                if ( it != pdn->parameters().begin() )
                    paramList += ",";
                paramList += "[" + slice(source, it->first) + "," + slice(source, it->second) + "]";
            }
        }

        *compose << indent(indentValue + 1) << ("Element.addEvent(this, \'" + slice(source, m_events[i]->name()) + "\', [" + paramList + "])\n");
    }

    for (size_t i = 0; i < m_listeners.size(); ++i)
    {
        std::string paramList = "";
        if ( m_listeners[i]->parameterList() ){
            ParameterListNode* pdn = m_listeners[i]->parameterList()->as<ParameterListNode>();
            for ( auto pit = pdn->parameters().begin(); pit != pdn->parameters().end(); ++pit ){
                if ( pit != pdn->parameters().begin() )
                    paramList += ",";
                paramList += slice(source, pit->second);
            }
        }

        *compose << "this.on(\'" << slice(source, m_listeners[i]->name()) << "\', function(" << paramList << ")";

        JSSection* jssection = new JSSection;
        jssection->from = m_listeners[i]->body()->startByte();
        jssection->to   = m_listeners[i]->body()->endByte();
        m_listeners[i]->convertToJs(source, jssection->m_children);

        std::vector<std::string> flat;
        jssection->flatten(source, flat);

        for (auto s: flat){
            *compose << s;
        }
        *compose << ".bind(this));\n";

    }

    for (size_t i = 0; i < m_properties.size(); ++i)
    {
        int numOfBindings = 0;
        if (m_properties[i]->hasBindings())
        {
            numOfBindings = m_properties[i]->bindings().size();
            std::string comp = indent(indentValue + 1) + "Element.assignPropertyExpression(this,\n"
                             + indent(indentValue + 1) + "'" + slice(source, m_properties[i]->name()) + "',\n";
            if (m_properties[i]->expression()){
                comp += "function(){ return " + slice(source, m_properties[i]->expression()) + "}.bind(this),\n";
            } else if (m_properties[i]->statementBlock()){
                auto block = m_properties[i]->statementBlock();
                comp += "(function()\n" ;
                el::JSSection* section = new el::JSSection;
                section->from = block->startByte();
                section->to = block->endByte();
                block->convertToJs(source, section->m_children);

                std::vector<std::string> flat;
                section->flatten(source, flat);
                for (auto s: flat) comp += s;
                    delete section;
                comp += ".bind(this)\n),\n";

            }
            comp += "[\n";

            std::set<std::pair<std::string, std::string>> bindingPairs;
            for (auto idx = m_properties[i]->bindings().begin(); idx != m_properties[i]->bindings().end(); ++idx)
            {
                BaseNode* node = *idx;
                if (node->typeString() == "MemberExpression")
                {
                    MemberExpressionNode* men = node->as<MemberExpressionNode>();
                    bool skip = false;


                    auto p = men->parent();
                    JsBlockNode* sb = nullptr;
                    while (p){
                        sb = dynamic_cast<JsBlockNode*>(p);
                        if (sb) break;
                        p = p->parent();
                    }

                    if (sb){
                        auto decl = sb->m_declarations;
                        if (men->children()[0]->typeString() == "Identifier"){
                            auto ident = dynamic_cast<IdentifierNode*>(men->children()[0]);
                            auto ids = slice(source, ident->startByte(), ident->endByte());
                            for (auto ch: decl){
                                auto localVar = slice(source, ch->startByte(), ch->endByte());

                                if (localVar == ids){
                                    --numOfBindings;
                                    skip = true;
                                    break;
                                }
                            }
                        }
                    }

                    if (!skip){
                        std::pair<std::string, std::string> pair = std::make_pair(
                            slice(source, men->children()[0]),
                            slice(source, men->children()[1])
                        );

                        if (bindingPairs.find(pair) == bindingPairs.end()){
                            if (!bindingPairs.empty()) comp += ",\n";
                            comp +=  "[ " + pair.first + ", '" +  pair.second + "Changed' ]";
                            bindingPairs.insert(pair);
                        }
                    }
                }
            }
            comp += "]\n)\n";
            if (numOfBindings > 0)
                *compose << comp;
        }
        if (numOfBindings == 0){
            *compose << "this." << slice(source,m_properties[i]->name())
                     << " = ";
            if (m_properties[i]->expression() && m_properties[i]->expression()->children().size() == 1
                    && (m_properties[i]->expression()->children()[0]->typeString() == "NewComponentExpression" || m_properties[i]->expression()->children()[0]->typeString() == "RootNewComponentExpression"))
            {
                auto nce = m_properties[i]->expression()->children()[0]->as<NewComponentExpressionNode>();

                el::JSSection* section = new el::JSSection;
                section->from = nce->startByte();
                section->to = nce->endByte();
                nce->convertToJs(source, section->m_children);
                std::vector<std::string> flat;
                section->flatten(source, flat);
                for (auto s: flat)
                    *compose << s;
                delete section;
            }
            else if (m_properties[i]->expression()){
                *compose << slice(source, m_properties[i]->expression()) << "\n";
            }
            else if (m_properties[i]->componentDeclaration())
            {
                auto cd = m_properties[i]->componentDeclaration();
                el::JSSection* section = new el::JSSection;
                section->from = cd->startByte();
                section->to = cd->endByte();
                cd->convertToJs(source, section->m_children);
                std::vector<std::string> flat;
                section->flatten(source, flat);
                for (auto s: flat) *compose << s;
                delete section;
            }
            else if (m_properties[i]->statementBlock())
            {
                auto block = m_properties[i]->statementBlock();
                *compose << "(function()\n" ;
                el::JSSection* section = new el::JSSection;
                section->from = block->startByte();
                section->to = block->endByte();
                block->convertToJs(source, section->m_children);
                std::vector<std::string> flat;
                section->flatten(source, flat);
                for (auto s: flat)
                    *compose << s;
                delete section;
                *compose << "\n)()\n\n";
            }

            *compose << "\n\n";
        }
    }

    for (size_t i = 0; i < m_assignments.size(); ++i){

        if (m_assignments[i]->m_bindings.size() > 0)
        {
            if (m_assignments[i]->m_expression)
            {
                auto& property = m_assignments[i]->m_property;
                std::string object = "this";
                for (size_t x = 0; x < property.size()-1; x++)
                {
                    object += "." + slice(source, property[x]);
                }
                *compose << "Element.assignPropertyExpression(" << object << ",\n"
                         << "'" << slice(source, property[property.size()-1])
                         << "',\n function(){ return " << slice(source, m_assignments[i]->m_expression) << "}.bind(" << object << "),\n"
                         << "[\n";
                std::set<std::pair<std::string, std::string>> bindingPairs;
                for (auto idx = m_assignments[i]->m_bindings.begin(); idx != m_assignments[i]->m_bindings.end(); ++idx)
                {
                    BaseNode* node = *idx;
                    if (node->typeString() == "MemberExpression")
                    {
                        MemberExpressionNode* men = node->as<MemberExpressionNode>();

                        std::pair<std::string, std::string> pair = std::make_pair(
                            slice(source, men->children()[0]),
                            slice(source, men->children()[1])
                        );

                        if (bindingPairs.find(pair) == bindingPairs.end())
                        {
                            if (!bindingPairs.empty()) *compose << ",\n";
                            *compose << "[ " << pair.first << ", '" <<  pair.second << "Changed' ]";
                            bindingPairs.insert(pair);
                        }

                    }
                }
                *compose << "]\n)\n";
            }
        }
        else {
            if (m_assignments[i]->m_expression && !m_assignments[i]->m_property.empty())
            {

                *compose << "this";

                for (size_t prop = 0; prop < m_assignments[i]->m_property.size(); ++prop){
                    *compose << "." << slice(source, m_assignments[i]->m_property[prop]);
                }

                auto exp = m_assignments[i]->m_expression;
                std::string comp = "" ;
                el::JSSection* section = new el::JSSection;
                section->from = exp->startByte();
                section->to = exp->endByte();
                exp->convertToJs(source, section->m_children);
                std::vector<std::string> flat;
                section->flatten(source, flat);
                for (auto s: flat) {
                    comp += s;
                }
                delete section;

                *compose << " = " << comp << "\n"; //slice(source, m_assignments[i]->m_expression) << "\n\n";
            }
            else if (m_assignments[i]->m_statementBlock) {
                std::string propName = "this";

                for (size_t prop = 0; prop < m_assignments[i]->m_property.size(); ++prop){
                    propName += "." + slice(source, m_assignments[i]->m_property[prop]);
                }
                *compose << propName << " = "
                         << "(function()" <<  slice(source, m_assignments[i]->m_statementBlock) << "())\n\n";
            }
        }

    }


    if (!m_nestedComponents.empty())
    {
        *compose << "Element.assignDefaultProperty(this, [\n";
        for (size_t i = 0; i < m_nestedComponents.size(); ++i)
        {
            if (i != 0)
                *compose << ",\n";
            el::JSSection* section = new el::JSSection;
            section->from = m_nestedComponents[i]->startByte();
            section->to = m_nestedComponents[i]->endByte();
            m_nestedComponents[i]->convertToJs(source, section->m_children);
            std::vector<std::string> flat;
            section->flatten(source, flat);
            for (auto s: flat) *compose << s;
            delete section;
        }
        *compose << "\n]\n)\n";
    }/* else {
        *compose << "Element.assignDefaultProperty(null)\n\n";
    }*/

    *compose << "}\n\n";

    // *compose << m_body->convertToJs(source);
    for ( auto it = m_body->children().begin(); it != m_body->children().end(); ++it ){

        BaseNode* c = *it;

        if ( c->typeString() == "MethodDefinition"){
            MethodDefinitionNode* mdn = c->as<MethodDefinitionNode>();

            JSSection* jssection = new JSSection;
            jssection->from = mdn->startByte();
            jssection->to   = mdn->endByte();
            *compose << jssection << "\n";
            mdn->body()->convertToJs(source, jssection->m_children);
        } else if ( c->typeString() == "TypedFunctionDeclaration" ){
            TypedFunctionDeclarationNode* tfdn = c->as<TypedFunctionDeclarationNode>();

            std::string paramList = "";
            if ( tfdn->parameters() ){
                ParameterListNode* pdn = tfdn->parameters()->as<ParameterListNode>();
                for ( auto pit = pdn->parameters().begin(); pit != pdn->parameters().end(); ++pit ){
                    if ( pit != pdn->parameters().begin() )
                        paramList += ",";
                    paramList += slice(source, pit->second);
                }
            }
            *compose << slice(source, tfdn->name()) << "(" << paramList << ")\n";

            JSSection* jssection = new JSSection;
            jssection->from = tfdn->body()->startByte();
            jssection->to   = tfdn->body()->endByte();
            tfdn->body()->convertToJs(source, jssection->m_children);
            std::vector<std::string> flat;
            jssection->flatten(source, flat);
            for (auto s: flat)
            {
                *compose << s << "\n";
            }
            delete jssection;

            *compose << "\n\n";
        }
    }

    *compose << "}\n";
    fragments.push_back(compose);
}

std::string ComponentDeclarationNode::name(const std::string& source) const{
    std::string name = slice(source, m_name);

    if (name == "default"){
        const BaseNode* p = this;
        while (p && p->typeString() != "Program")
            p = p->parent();
        if (p) {
            const ProgramNode* program = dynamic_cast<const ProgramNode*>(p);
            if (program)
                name = program->filename();
        }
    }

    return name;
}


void NewComponentExpressionNode::convertToJs(const std::string &source, std::vector<ElementsInsertion *> &fragments, int indt){

    ElementsInsertion* compose = new ElementsInsertion;
    compose->from = startByte();
    compose->to   = endByte();

    if (m_instance){
        *compose << "export let " << name(source) << " = ";
    }
    *compose << indent(indt) << "(function(parent){\n" << indent(indt + 1) << "this.setParent(parent)\n";

    std::string id_root = "this";
    bool isRoot = (dynamic_cast<RootNewComponentExpressionNode*>(this) != nullptr);
    if (m_id) {
        *compose << indent(indt + 1) << "Element.assignId(" << slice(source,m_id) << ", \"" << slice(source,m_id) << "\")\n";
        if (isRoot){
            id_root = slice(source,m_id);
            *compose << indent(indt + 1) << "var " << id_root << " = this\n\n";
        }
    }


    if (isRoot && !m_idComponents.empty()){
        for (size_t i = 0; i < m_idComponents.size();++i)
        {
            auto type = m_idComponents[i]->initializerName(source);
            *compose << indent(indt + 1) << "var " << slice(source, m_idComponents[i]->id()) << " = new " << type;
            if (m_idComponents[i]->arguments())
                *compose << slice(source, m_idComponents[i]->arguments()) << "\n";
            else
                *compose << "()\n";
        }
    }

    if (isRoot || !m_id){
        for (size_t i = 0; i < m_properties.size(); ++i)
        {
            *compose << indent(indt + 1) << "Element.addProperty(" + id_root + ", '" << slice(source, m_properties[i]->name())
                     << "', { type: '" << slice(source, m_properties[i]->type()) << "', notify: '"
                     << slice(source, m_properties[i]->name()) << "Changed' })\n";
        }
    }

    if (isRoot && !m_idComponents.empty()){
        for (size_t i = 0; i < m_idComponents.size();++i)
        {
            std::string id = slice(source, m_idComponents[i]->id());
            auto properties = m_idComponents[i]->properties();

            for (size_t idx = 0; idx < properties.size(); ++idx)
            {
                *compose << indent(indt + 1) << "Element.addProperty(" << id << ", '" << slice(source, properties[idx]->name())
                         << "', { type: '" << slice(source, properties[idx]->type()) << "', notify: '"
                         << slice(source, properties[idx]->name()) << "Changed' })\n";
            }

        }

    }

    for (size_t i = 0; i < m_properties.size(); ++i)
    {
        int numOfBindings = 0;
        if (m_properties[i]->hasBindings())
        {
            std::string comp = "";
            numOfBindings = m_properties[i]->bindings().size();
            if (m_properties[i]->expression())
            {
                comp += indent(indt + 1) + "Element.assignPropertyExpression(this,\n"
                      + indent(indt + 2) + "'" + slice(source, m_properties[i]->name()) + "',\n"
                      + indent(indt + 2) + "function(){ return " + slice(source, m_properties[i]->expression()) + "}.bind(this),\n"
                      + indent(indt + 2) + "[\n";
                std::set<std::pair<std::string, std::string>> bindingPairs;
                for (auto idx = m_properties[i]->bindings().begin(); idx != m_properties[i]->bindings().end(); ++idx)
                {
                    BaseNode* node = *idx;
                    if (node->typeString() == "MemberExpression")
                    {
                        MemberExpressionNode* men = node->as<MemberExpressionNode>();

                        std::pair<std::string, std::string> pair = std::make_pair(
                            slice(source, men->children()[0]),
                            slice(source, men->children()[1])
                        );

                        if (bindingPairs.find(pair) == bindingPairs.end())
                        {
                            if (!bindingPairs.empty())
                                comp += ",\n";
                            comp += indent(indt + 3) + "[ " + pair.first + ", '" +  pair.second + "Changed' ]";
                            bindingPairs.insert(pair);
                        }

                    }
                }
                comp += indent(indt + 1) + "]\n)\n";
            } else {
                comp += indent(indt + 1) + "Element.assignPropertyExpression(this,\n"
                      + indent(indt + 2) + "'" + slice(source, m_properties[i]->name()) + "',\n" + indent(indt + 2) + "function()";
                el::JSSection* section = new el::JSSection;
                auto block = m_properties[i]->statementBlock();
                section->from = block->startByte();
                section->to = block->endByte();
                block->convertToJs(source, section->m_children);
                std::vector<std::string> flat;
                section->flatten(source, flat);
                for (auto s: flat) comp += s;
                delete section;
                comp += ".bind(this),\n" + indent(indt + 1) + "[\n";
                std::set<std::pair<std::string, std::string>> bindingPairs;
                for (auto idx = m_properties[i]->bindings().begin(); idx != m_properties[i]->bindings().end(); ++idx)
                {
                    BaseNode* node = *idx;
                    if (node->typeString() == "MemberExpression")
                    {
                        MemberExpressionNode* men = node->as<MemberExpressionNode>();

                        bool skip = false;

                        auto p = men->parent();
                        JsBlockNode* sb = nullptr;
                        while (p){
                            sb = dynamic_cast<JsBlockNode*>(p);
                            if (sb) break;
                            p = p->parent();
                        }

                        if (sb){
                            auto decl = sb->m_declarations;
                            if (men->children()[0]->typeString() == "Identifier"){
                                auto ident = dynamic_cast<IdentifierNode*>(men->children()[0]);
                                auto ids = slice(source, ident->startByte(), ident->endByte());
                                for (auto ch: decl){
                                    auto localVar = slice(source, ch->startByte(), ch->endByte());

                                    if (localVar == ids){
                                        --numOfBindings;
                                        skip = true;
                                        break;
                                    }
                                }
                            }
                        }

                        if (!skip){
                            std::pair<std::string, std::string> pair = std::make_pair(
                                slice(source, men->children()[0]),
                                slice(source, men->children()[1])
                            );

                            if (bindingPairs.find(pair) == bindingPairs.end())
                            {
                                if (!bindingPairs.empty()) comp += ",\n";
                                comp +=  "[ " + pair.first + ", '" +  pair.second + "Changed' ]";
                                bindingPairs.insert(pair);
                            }
                        }

                    }
                }
                comp += "\n" + indent(indt + 1) + "]\n)\n";
            }

            if (numOfBindings > 0)
                *compose << comp;
        }
        if (numOfBindings == 0){
            if (m_properties[i]->expression())
            {
                auto expr = m_properties[i]->expression();
                *compose << indent(indt + 1) << "this." << slice(source,m_properties[i]->name()) << " = ";

                // convert the subexpression
                JSSection* expressionSection = new JSSection;
                expressionSection->from = expr->startByte();
                expressionSection->to   = expr->endByte();
                expr->convertToJs(source, expressionSection->m_children);
                std::vector<std::string> flat;
                expressionSection->flatten(source, flat);
                for (auto s: flat) *compose << s;

                *compose << "\n";
            }
            else if (m_properties[i]->statementBlock()) {
                *compose << indent(indt + 1) << "this." << slice(source,m_properties[i]->name()) << " = " << "(function()";
                el::JSSection* section = new el::JSSection;
                auto block = m_properties[i]->statementBlock();
                section->from = block->startByte();
                section->to = block->endByte();
                block->convertToJs(source, section->m_children);
                std::vector<std::string> flat;
                section->flatten(source, flat);
                for (auto s: flat) *compose <<  s;
                delete section;

                *compose << "())\n\n";
            }
        }
    }

    for (size_t i = 0; i < m_assignments.size(); ++i){

        if (m_assignments[i]->m_bindings.size() > 0){

            if (m_assignments[i]->m_expression){

                auto& property = m_assignments[i]->m_property;
                std::string object = "this";
                for (size_t x = 0; x < property.size()-1; x++)
                {
                    object += "." + slice(source, property[x]);
                }
                *compose << indent(indt + 1) << "Element.assignPropertyExpression(" << object << ",\n"
                         << indent(indt + 2) << "'" << slice(source, property[property.size()-1])
                         << "',\n function(){ return " << slice(source, m_assignments[i]->m_expression) << "}.bind(" << object << "),\n"
                         << "[\n";
                std::set<std::pair<std::string, std::string>> bindingPairs;
                for (auto idx = m_assignments[i]->m_bindings.begin(); idx != m_assignments[i]->m_bindings.end(); ++idx)
                {
                    BaseNode* node = *idx;
                    if (node->typeString() == "MemberExpression")
                    {
                        MemberExpressionNode* men = node->as<MemberExpressionNode>();

                        std::pair<std::string, std::string> pair = std::make_pair(
                            slice(source, men->children()[0]),
                            slice(source, men->children()[1])
                        );

                        if (bindingPairs.find(pair) == bindingPairs.end())
                        {
                            if (!bindingPairs.empty()) *compose << ",\n";
                            *compose << "[ " << pair.first << ", '" <<  pair.second << "Changed' ]";
                            bindingPairs.insert(pair);
                        }

                    }
                }
                *compose << indent(indt + 1) << "]\n)\n";
            }
        } else {
            if (m_assignments[i]->m_expression && !m_assignments[i]->m_property.empty()){
                *compose << indent(indt + 1) << "this";

                for (size_t prop = 0; prop < m_assignments[i]->m_property.size(); ++prop)
                {
                    *compose << "." << slice(source, m_assignments[i]->m_property[prop]);
                }

                *compose << " = " << slice(source, m_assignments[i]->m_expression) << "\n\n";
            } else if (m_assignments[i]->m_statementBlock) {
                std::string propName = "this";

                for (size_t prop = 0; prop < m_assignments[i]->m_property.size(); ++prop)
                {
                    propName += "." + slice(source, m_assignments[i]->m_property[prop]);
                }
                *compose << indent(indt + 1) << propName << " = "
                         << "(function()" <<  slice(source, m_assignments[i]->m_statementBlock) << "())\n\n";
            }
        }
    }

    if (!m_nestedComponents.empty())
    {
        *compose << indent(indt + 1) << "Element.assignDefaultProperty(this, [\n";
        for (unsigned i = 0; i < m_nestedComponents.size(); ++i)
        {
            if (i != 0) *compose << ",\n";
            el::JSSection* section = new el::JSSection;
            section->from = m_nestedComponents[i]->startByte();
            section->to = m_nestedComponents[i]->endByte();
            m_nestedComponents[i]->convertToJs(source, section->m_children);
            std::vector<std::string> flat;
            section->flatten(source, flat);
            for (auto s: flat) *compose << s << "\n";
            delete section;
        }
        *compose << indent(indt + 1) << "\n]" << indent(indt) << ")\n";
    }/* else {
        *compose << "Element.assignDefaultProperty(null)\n\n";
    }*/


    for ( auto it = m_body->children().begin(); it != m_body->children().end(); ++it ){
        BaseNode* c = *it;

        if ( c->typeString() == "EventDeclaration"){
            EventDeclarationNode* edn = c->as<EventDeclarationNode>();

            std::string paramList = "";
            if ( edn->parameterList() ){
                ParameterListNode* pdn = edn->parameterList()->as<ParameterListNode>();
                for ( auto it = pdn->parameters().begin(); it != pdn->parameters().end(); ++it ){
                    if ( it != pdn->parameters().begin() )
                        paramList += ",";
                    paramList += "[" + slice(source, it->first) + "," + slice(source, it->second) + "]";
                }
            }

            *compose << indent(indt + 1) << ("Element.addEvent(this, \'" + slice(source, edn->name()) + "\', [" + paramList + "])\n");
        } else if ( c->typeString() == "ListenerDeclaration" ){
            ListenerDeclarationNode* ldn = c->as<ListenerDeclarationNode>();

            std::string paramList = "";
            if ( ldn->parameterList() ){
                ParameterListNode* pdn = ldn->parameterList()->as<ParameterListNode>();
                for ( auto pit = pdn->parameters().begin(); pit != pdn->parameters().end(); ++pit ){
                    if ( pit != pdn->parameters().begin() )
                        paramList += ",";
                    paramList += slice(source, pit->second);
                }
            }

            *compose << indent(indt + 1) << "this.on(\'" << slice(source, ldn->name()) << "\', function(" << paramList << ")";
            JSSection* jssection = new JSSection;
            jssection->from = ldn->body()->startByte();
            jssection->to   = ldn->body()->endByte();
            ldn->convertToJs(source, jssection->m_children, indt + 1);
            std::vector<std::string> flat;
            jssection->flatten(source, flat);
            for (auto s: flat) *compose << s;
            *compose << ".bind(this));\n";

        } else if ( c->typeString() == "MethodDefinition"){
            MethodDefinitionNode* mdn = c->as<MethodDefinitionNode>();

            JSSection* jssection = new JSSection;
            jssection->from = mdn->startByte();
            jssection->to   = mdn->endByte();
            *compose << jssection;
            mdn->body()->convertToJs(source, jssection->m_children);
        } else if ( c->typeString() == "TypedFunctionDeclaration" ){
            TypedFunctionDeclarationNode* tfdn = c->as<TypedFunctionDeclarationNode>();

            std::string paramList = "";
            if ( tfdn->parameters() ){
                ParameterListNode* pdn = tfdn->parameters()->as<ParameterListNode>();
                for ( auto pit = pdn->parameters().begin(); pit != pdn->parameters().end(); ++pit ){
                    if ( pit != pdn->parameters().begin() )
                        paramList += ",";
                    paramList += slice(source, pit->second);
                }
            }
            JSSection* jssection = new JSSection;
            jssection->from = tfdn->body()->startByte();
            jssection->to   = tfdn->body()->endByte();
            *compose << indent(indt + 1) << "this." << slice(source, tfdn->name()) << " = function(" << paramList << ")" << jssection << "\n";

            tfdn->body()->convertToJs(source, jssection->m_children);
        }
    }

    *compose << indent(indt + 1) << "return this\n" << indent(indt) << "}.bind(";

    if (!m_id || isRoot)
    {
        *compose << "new ";
        std::string name;
        for ( auto nameIden : m_name ){
            if ( !name.empty() )
                name += ".";
            name += slice(source, nameIden);
        }
        *compose << name;

        if (!m_arguments)
            *compose << "()";
        else
            *compose << slice(source, m_arguments);
    } else {
        *compose << slice(source, m_id);
    }

    bool isThis = parent() && parent()->typeString() == "ComponentBody";
    isThis = isThis || (parent()
                        && parent()->parent()
                        && parent()->parent()->typeString() == "PropertyDeclaration"
                        && parent()->parent()->parent()
                        && parent()->parent()->parent()->typeString() == "ComponentBody" );

    if (isThis)
    {
        *compose << ")(this)";
    } else {
        *compose << ")(null)";
    }

    *compose << ")";

    fragments.push_back(compose);
}

std::string NewComponentExpressionNode::name(const std::string &source) const{
    if ( !m_instance )
        return "";

    std::string instance_name = slice(source, m_instance);

    if (instance_name == "default"){
        const BaseNode* p = this;
        while (p && p->typeString() != "Program")
            p = p->parent();
        if (p) {
            const ProgramNode* program = dynamic_cast<const ProgramNode*>(p);
            if (program) instance_name =
                    program->filename();
        }
    }

    return instance_name;
}


NewComponentExpressionNode::NewComponentExpressionNode(const TSNode &node, const std::string& typeString)
    : JsBlockNode(node, typeString)
    , m_id(nullptr)
    , m_body(nullptr)
    , m_arguments(nullptr)
    , m_instance(nullptr)
{

}

NewComponentExpressionNode::NewComponentExpressionNode(const TSNode &node):
    NewComponentExpressionNode(node, "NewComponentExpression")
{

}


std::string NewComponentExpressionNode::initializerName(const std::string &source){
    std::string name;
    for ( auto nameIden : m_name ){
        if ( !name.empty() )
            name += ".";
        name += slice(source, nameIden);
    }
    return name;
}

std::string NewComponentExpressionNode::toString(int indent) const{
    std::string result;
    if ( indent > 0 )
        result.assign(indent * 2, ' ');

    std::string name =
        "(name " +
        (!m_name.empty()
            ? std::string("[") + std::to_string(m_name.front()->startByte()) + ", " +
              std::to_string(m_name.back()->endByte()) + "]" : "") + ")";

    std::string identifier = "";
    if ( m_id )
        identifier = "(id " + m_id->rangeString() + ")";


    std::string args = "";
    if (m_arguments)
        args = "(args " + m_arguments->rangeString() + ")";

    result += typeString() + " " + name + identifier +args + rangeString() + "\n";
    if ( m_body ){
        result += m_body->toString(indent >= 0 ? indent + 1 : indent);
    }

    if (m_arguments)
        result += m_arguments->toString(indent >= 0 ? indent + 1 : indent);

    return result;
}

std::string ComponentBodyNode::toString(int indent) const{
    std::string result;
    if ( indent > 0 )
        result.assign(indent * 2, ' ');

    result += "ComponentBody " + rangeString() + "\n";

    const std::vector<BaseNode*>& c = children();
    for ( BaseNode* node : c ){
        result += node->toString(indent >= 0 ? indent + 1 : indent);
    }

    return result;
}

PropertyDeclarationNode::PropertyDeclarationNode(const TSNode &node)
    : JsBlockNode(node, "PropertyDeclaration")
    , m_name(nullptr)
    , m_type(nullptr)
    , m_expression(nullptr)
    , m_statementBlock(nullptr)
    , m_componentDeclaration(nullptr)
{
}

std::string PropertyDeclarationNode::toString(int indent) const{
    std::string result;
    if ( indent > 0 )
        result.assign(indent * 2, ' ');

    std::string name = "";
    if ( m_name )
        name = "(name " + m_name->rangeString() + ")";
    std::string type = "";
    if ( m_type )
        type = "(type " + m_type->rangeString() + ")";
    result += "PropertyDeclaration " + rangeString() + name + type + "\n";

    if ( m_expression )
        result += m_expression->toString(indent >= 0 ? indent + 1 : indent);

    if ( m_statementBlock )
        result += m_statementBlock->toString(indent >= 0 ? indent + 1 : indent);

    return result;
}

PropertyAssignmentNode::PropertyAssignmentNode(const TSNode &node)
    : JsBlockNode(node, "PropertyAssignment")
    , m_expression(nullptr)
{
}

std::string PropertyAssignmentNode::toString(int indent) const{
    std::string result;
   if ( indent > 0 )
       result.assign(indent * 2, ' ');
    std::string name = "";
//    if ( m_property )
//        name = "(property " + m_property->rangeString() + ")";
    result += "PropertyAssignment " + rangeString() + name + "\n";
    if ( m_expression )
        result += m_expression->toString(indent >= 0 ? indent + 1 : indent);
    return result;
}

EventDeclarationNode::EventDeclarationNode(const TSNode &node)
    : BaseNode(node, "EventDeclaration")
    , m_name(nullptr)
    , m_parameters(nullptr)
{}

std::string EventDeclarationNode::toString(int indent) const{
    std::string result;
    if ( indent > 0 )
        result.assign(indent * 2, ' ');
    std::string name = "";
    if ( m_name )
        name = "(name " + m_name->rangeString() + ")";
    result += "EventDeclaration " + rangeString() + name + "\n";
    if ( m_parameters )
        result += m_parameters->toString(indent >= 0 ? indent + 1 : indent);
    return result;
}

std::string ParameterListNode::toString(int indent) const{
    std::string result;
   if ( indent > 0 )
       result.assign(indent * 2, ' ');

    std::string parameters = "(no parameters: " + std::to_string(m_parameters.size()) + ")";

    result += "ParameterList " + rangeString() + parameters + "\n";
    return result;
}

ListenerDeclarationNode::ListenerDeclarationNode(const TSNode &node)
    : BaseNode(node, "ListenerDeclaration")
    , m_name(nullptr)
    , m_parameters(nullptr)
    , m_body(nullptr)
{}

std::string ListenerDeclarationNode::toString(int indent) const{
    std::string result;
    if ( indent > 0 )
        result.assign(indent * 2, ' ');
    std::string name = "";
    if ( m_name )
        name = "(name " + m_name->rangeString() + ")";

    result += "ListenerDeclaration " + rangeString() + name + "\n";
    if ( m_parameters )
        result += m_parameters->toString(indent >= 0 ? indent + 1 : indent);
    if ( m_body )
        result += m_body->toString(indent >= 0 ? indent + 1 : indent);

    return result;
}

MethodDefinitionNode::MethodDefinitionNode(const TSNode &node)
    : BaseNode(node, "MethodDefinition")
    , m_name(nullptr)
    , m_parameters(nullptr)
    , m_body(nullptr)
{
}

std::string MethodDefinitionNode::toString(int indent) const{
    std::string result;
    if ( indent > 0 )
        result.assign(indent * 2, ' ');
    std::string name = "";
    if ( m_name )
        name = "(name " + m_name->rangeString() + ")";

    result += "MethodDefinition " + rangeString() + name + "\n";
    if ( m_parameters )
        result += m_parameters->toString(indent >= 0 ? indent + 1 : indent);
    if ( m_body )
        result += m_body->toString(indent >= 0 ? indent + 1 : indent);

    return result;
}


ArrowFunctionNode::ArrowFunctionNode(const TSNode &node)
    : BaseNode(node, "ArrowFunction")
    , m_name(nullptr)
    , m_body(nullptr)
{
}

std::string ArrowFunctionNode::toString(int indent) const{
    std::string result;
    if ( indent > 0 )
        result.assign(indent * 2, ' ');
    std::string name = "";
    if ( m_name )
        name = "(name " + m_name->rangeString() + ")";

    result += "ArrowFunction " + rangeString() + name + "\n";

    if ( m_parameters.size() > 0 ){
        std::string paramsResult;
        if ( indent > 0 )
            paramsResult.assign(indent * 2, ' ');
        std::string parameters = "(no parameters: " + std::to_string(m_parameters.size()) + ")";
        result += paramsResult + "ParameterList " + rangeString() + parameters + "\n";
    }

    if ( m_body )
        result += m_body->toString(indent >= 0 ? indent + 1 : indent);

    return result;
}


FunctionDeclarationNode::FunctionDeclarationNode(const TSNode &node)
    : BaseNode(node, "FunctionDeclaration")
    , m_name(nullptr)
    , m_body(nullptr)
{
}

std::string FunctionDeclarationNode::toString(int indent) const{
    std::string result;
    if ( indent > 0 )
        result.assign(indent * 2, ' ');
    std::string name = "";
    if ( m_name )
        name = "(name " + m_name->rangeString() + ")";

    result += "ArrowFunction " + rangeString() + name + "\n";

    if ( m_parameters.size() > 0 ){
        std::string paramsResult;
        if ( indent > 0 )
            paramsResult.assign(indent * 2, ' ');
        std::string parameters = "(no parameters: " + std::to_string(m_parameters.size()) + ")";
        result += paramsResult + "ParameterList " + rangeString() + parameters + "\n";
    }

    if ( m_body )
        result += m_body->toString(indent >= 0 ? indent + 1 : indent);

    return result;
}



TypedFunctionDeclarationNode::TypedFunctionDeclarationNode(const TSNode &node)
    : BaseNode(node, "TypedFunctionDeclaration")
    , m_name(nullptr)
    , m_parameters(nullptr)
    , m_body(nullptr)
{
}

std::string TypedFunctionDeclarationNode::toString(int indent) const{
    std::string result;
    if ( indent > 0 )
        result.assign(indent * 2, ' ');
    std::string name = "";
    if ( m_name )
        name = "(name " + m_name->rangeString() + ")";

    result += "TypedFunctionDeclaration " + rangeString() + name + "\n";
    if ( m_parameters )
        result += m_parameters->toString(indent >= 0 ? indent + 1 : indent);
    if ( m_body )
        result += m_body->toString(indent >= 0 ? indent + 1 : indent);

    return result;
}

std::string ConstructorDefinitionNode::toString(int indent) const
{
    std::string result;
    if ( indent > 0 )
        result.assign(indent * 2, ' ');

    result += "ConstructorDefinition " + rangeString() + "\n";


    if (m_formalParameters)
        result += m_formalParameters->toString(indent >= 0 ? indent + 1 : indent);

    if (m_block)
        result += m_block->toString(indent >= 0 ? indent + 1 : indent);

    for (auto it = children().begin(); it != children().end(); ++it)
        result += (*it)->toString(indent >= 0 ? indent + 1 : indent);



    return result;
}

std::string ExpressionStatementNode::toString(int indent) const
{
    std::string result;
    if ( indent > 0 )
        result.assign(indent * 2, ' ');

    result += "ExpressionStatement " + rangeString() + "\n";
    for (auto it = children().begin(); it != children().end(); ++it)
        result += (*it)->toString(indent >= 0 ? indent + 1 : indent);

    return result;
}

std::string CallExpressionNode::toString(int indent) const
{
    std::string result;
    if ( indent > 0 )
        result.assign(indent * 2, ' ');

    result += "CallExpression " + rangeString() + (isSuper ? " SUPER" : "") + "\n";
    if (m_arguments)
        result += m_arguments->toString(indent >= 0 ? indent + 1 : indent);

    return result;
}

std::string NewTaggedComponentExpressionNode::toString(int indent) const
{
    std::string result;
    if ( indent > 0 )
        result.assign(indent * 2, ' ');

    result += "NewTaggedComponentExpression " + rangeString() + "\n";

    for (auto it = children().begin(); it != children().end(); ++it)
        result += (*it)->toString(indent >= 0 ? indent + 1 : indent);

    return result;
}

void NewTaggedComponentExpressionNode::convertToJs(const std::string &source, std::vector<ElementsInsertion *> &fragments, int indentValue)
{
    ElementsInsertion* compose = new ElementsInsertion;
    compose->from = startByte();
    compose->to = endByte();
    std::string name, value;
    for (auto child: children())
    {
        if (child->typeString() == "Identifier") {
            name = slice(source, child);
        }
        else
        {
            value = slice(source, child);
            value = value.substr(1, value.length() - 2);

            std::replace(value.begin(), value.end(), '\r', ' ');
            std::replace(value.begin(), value.end(), '\n', ' ');
            std::replace(value.begin(), value.end(), '\t', ' ');

            Utf8::trimRight(value);

            std::string result = "";
            size_t i = 0;
            while (i < value.length())
            {
                if (value[i] == ' ')
                {
                    while (value[i] == ' ') ++i;
                    result += " ";
                } else if (value[i] == '\\') {
                    if (i == value.length() -1) break;
                    ++i;
                    if (value[i] == 'n') result += "\\n";
                    if (value[i] == 's') result += ' ';
                    if (value[i] == '\\') result += "\\\\";
                    ++i;
                } else {
                    result += value[i];
                    ++i;
                }

            }

            value = result;
        }
    }

    *compose << indent(indentValue + 1) << "(function(parent){\n";
    *compose << indent(indentValue + 2) << "this.setParent(parent)\n";
    *compose << indent(indentValue + 2) << "return this\n";
    *compose << indent(indentValue + 1) << "}.bind(new " << name << "(\"" << value << "\"))(this))\n";

    fragments.push_back(compose);
}

std::string ArgumentsNode::toString(int indent) const
{
    std::string result;
    if ( indent > 0 )
        result.assign(indent * 2, ' ');

    result += "Arguments " + rangeString() + "\n";

    for (auto it = children().begin(); it != children().end(); ++it)
        result += (*it)->toString(indent >= 0 ? indent + 1 : indent);

    return result;
}

std::string VariableDeclarationNode::toString(int indent) const
{
    std::string result;
    if ( indent > 0 )
        result.assign(indent * 2, ' ');

    result += "VariableDeclaration " + rangeString() + "\n";

    for (size_t i = 0; i < m_declarators.size(); i++)
    {
        result += m_declarators[i]->toString(indent >= 0 ? indent + 1 : indent);
    }

    for (auto it = children().begin(); it != children().end(); ++it)
        result += (*it)->toString(indent >= 0 ? indent + 1 : indent);

    return result;
}

void VariableDeclarationNode::convertToJs(const std::string &source, std::vector<ElementsInsertion *> &fragments, int)
{
    ElementsInsertion* compose = new ElementsInsertion;
    compose->from = startByte();
    compose->to   = endByte();

    for (auto decl: m_declarators){
        auto children = decl->children();
        if (children.size() == 2 && children[0]->typeString() == "Identifier" && (children[1]->typeString() == "NewComponentExpression" || children[1]->typeString() == "RootNewComponentExpression")){
            *compose << "\nvar " << slice(source, children[0]->startByte(), children[0]->endByte()) << " = ";

            el::JSSection* section = new el::JSSection;
            section->from = children[1]->startByte();
            section->to = children[1]->endByte();
            children[1]->convertToJs(source, section->m_children);
            std::vector<std::string> flat;
            section->flatten(source, flat);
            for (auto s: flat) *compose << s;
            delete section;
        } else {
            *compose << "\nvar " << slice(source, decl->startByte(), decl->endByte());
        }
    }
    *compose << (m_hasSemicolon ? "; ": "\n");

    fragments.push_back(compose);
}

void JsBlockNode::collectImports(const std::string &source, std::vector<IdentifierNode *> &identifiers){
    BaseNode::collectImports(source, identifiers);
    collectBlockImports(source, identifiers);
}

void JsBlockNode::collectBlockImports(const std::string &source, std::vector<IdentifierNode *> &identifiers){
    for ( auto identifier : m_usedIdentifiers ){
        if ( !checkIdentifierDeclared(source, this, slice(source, identifier)) ){
            identifiers.push_back(identifier);
        }
    }
}


}} // namespace lv, el

