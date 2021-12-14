#include "modulefile.h"
#include "imports_p.h"
#include "languagenodes_p.h"
#include "live/elements/languageparser.h"
#include "live/exception.h"
#include "live/plugin.h"
#include "live/plugincontext.h"
#include "live/packagegraph.h"
#include "live/elements/engine.h"
#include "live/elements/script.h"

#include <sstream>

namespace lv{ namespace el{

class ModuleFilePrivate{
public:
    std::string name;
    std::string content;
    ElementsModule::Ptr plugin;
    ProgramNode* rootNode;
    JsModule::Ptr jsModule;

    ModuleFile::State state;

    std::list<ModuleFile::Export> exports;
    std::list<ModuleFile::Import> imports;

    std::list<ModuleFile*> dependencies;
    std::list<ModuleFile*> dependents;

    Script::Ptr script;
    Imports* importsObject;
};

ModuleFile::~ModuleFile(){
    delete m_d->importsObject;
    delete m_d;
}

ScopedValue ModuleFile::get(Engine* engine, ModuleFile *from, const std::string &name){
    return ScopedValue(engine);
}

void ModuleFile::parse(Engine* engine){
    if ( !engine )
        return;
    std::string fp = filePath();
    //if ( engine->moduleFileType() == Engine::JsOnly ){
        fp += ".js";
    //}
//    m_d->exports = engine->parser()->parseExportNames(fp);
        //    m_d->state = ModuleFile::Parsed;
}

void ModuleFile::resolveTypes(){
    auto impTypes = m_d->rootNode->importTypes();
    for ( auto nsit = impTypes.begin(); nsit != impTypes.end(); ++nsit ){
        for ( auto it = nsit->second.begin(); it != nsit->second.end(); ++it ){
            ProgramNode::ImportType& impType = it->second;
            bool foundLocalExport = false;
            if ( impType.importNamespace.empty() ){
                auto foundExp = m_d->plugin->findExport(impType.name);
                if ( foundExp.isValid() ){
                    addDependency(foundExp.file);

                    m_d->rootNode->resolveImport(impType.importNamespace, impType.name, "./" + foundExp.file->jsFileName());
                    foundLocalExport = true;
                }
            }

            if ( !foundLocalExport ){
                for( auto impIt = m_d->imports.begin(); impIt != m_d->imports.end(); ++impIt ){
                    if ( impIt->as == impType.importNamespace ){
                        auto foundExp = impIt->module->findExport(impType.name);
                        if ( foundExp.isValid() ){
                            if ( impIt->isRelative ){
                                // this plugin to package
                                std::vector<Utf8> parts = Utf8(m_d->plugin->plugin()->context()->importId).split(".");
                                if ( parts.size() > 0 ){
                                    parts.erase(parts.begin());
                                }

                                std::string pluginToPackage = "";
                                if ( parts.size() > 0 ){
                                    for ( size_t i = 0; i < parts.size(); ++i ){
                                        if ( !pluginToPackage.empty() )
                                            pluginToPackage += '/';
                                        pluginToPackage += "..";
                                    }
                                } else {
                                    pluginToPackage = ".";
                                }

                                // package to new plugin

                                std::vector<Utf8> packageToNewPluginParts = Utf8(impIt->module->plugin()->context()->importId).split(".");
                                if ( packageToNewPluginParts.size() > 0 ){
                                    packageToNewPluginParts.erase(packageToNewPluginParts.begin());
                                }
                                std::string packageToNewPlugin = Utf8::join(packageToNewPluginParts, "/").data();
                                if ( !packageToNewPlugin.empty() )
                                    packageToNewPlugin += "/";
                                m_d->rootNode->resolveImport(impType.importNamespace, impType.name, pluginToPackage + "/" + packageToNewPlugin + foundExp.file->jsFileName());

                            } else {
                                m_d->rootNode->resolveImport(impType.importNamespace, impType.name, impIt->module->plugin()->context()->importId + "/" + foundExp.file->jsFileName());
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
}

void ModuleFile::compile(){
    std::string jsContents = m_d->plugin->engine()->parser()->toJs(m_d->content, m_d->rootNode);
    m_d->plugin->engine()->fileInterceptor()->writeToFile(filePath() + ".js", jsContents);
}

ModuleFile::State ModuleFile::state() const{
    return m_d->state;
}

const std::string &ModuleFile::name() const{
    return m_d->name;
}

std::string ModuleFile::fileName() const{
    return m_d->name + ".lv";
}

std::string ModuleFile::jsFileName() const{
    return fileName() + ".js";
}

std::string ModuleFile::jsFilePath() const{
    return m_d->plugin->plugin()->path() + "/" + jsFileName();
}

std::string ModuleFile::filePath() const{
    return m_d->plugin->plugin()->path() + "/" + fileName();
}

const ElementsModule::Ptr &ModuleFile::plugin() const{
    return m_d->plugin;
}

const std::list<ModuleFile::Export> &ModuleFile::exports() const{
    return m_d->exports;
}

const std::list<ModuleFile::Import> &ModuleFile::imports() const{
    return m_d->imports;
}

void ModuleFile::resolveImport(const std::string &uri, ElementsModule::Ptr epl){
    for ( auto it = m_d->imports.begin(); it != m_d->imports.end(); ++it ){
        if ( it->uri == uri )
            it->module = epl;
    }
}

Imports *ModuleFile::importsObject(){
    return m_d->importsObject;
}

ScopedValue ModuleFile::createObject(Engine* engine) const{
    return ScopedValue(engine);
//    v8::Local<v8::Object> obj = v8::Object::New(engine->isolate());
//    v8::Local<v8::Value> exports = ScopedValue(engine, *m_d->exports).data();

//    auto isolate = engine->isolate();
//    auto context = isolate->GetCurrentContext();
//    v8::Local<v8::String> exportsKey = v8::String::NewFromUtf8(
//        engine->isolate(), "exports", v8::NewStringType::kInternalized
//    ).ToLocalChecked();
//    v8::Local<v8::String> pathKey = v8::String::NewFromUtf8(
//        engine->isolate(), "path", v8::NewStringType::kInternalized
//    ).ToLocalChecked();
//    v8::Local<v8::String> pathStr = v8::String::NewFromUtf8(
//        engine->isolate(), filePath().c_str(), v8::NewStringType::kInternalized
//    ).ToLocalChecked();

//    obj->Set(context, exportsKey, exports).IsNothing();
//    obj->Set(context, pathKey, pathStr).IsNothing();

    //    return ScopedValue(obj);
}

void ModuleFile::addDependency(ModuleFile *dependency){
    if ( dependency == this )
        return;
    m_d->dependencies.push_back(dependency);
    dependency->m_d->dependents.push_back(this);

    PackageGraph::CyclesResult<ModuleFile*> cr = checkCycles(this);
    if ( cr.found() ){
        std::stringstream ss;

        for ( auto it = cr.path().begin(); it != cr.path().end(); ++it ){
            ModuleFile* n = *it;
            if ( it != cr.path().begin() )
                ss << " -> ";
            ss << n->name();
        }

        for ( auto it = m_d->dependencies.begin(); it != m_d->dependencies.end(); ++it ){
            if ( *it == dependency ){
                m_d->dependencies.erase(it);
                break;
            }
        }
        for ( auto it = dependency->m_d->dependents.begin(); it != dependency->m_d->dependents.end(); ++it ){
            if ( *it == this ){
                dependency->m_d->dependents.erase(it);
                break;
            }
        }

        THROW_EXCEPTION(lv::Exception, "Module file dependency cycle found: "  + ss.str(), lv::Exception::toCode("Cycle"));
    }
}

void ModuleFile::setJsModule(const JsModule::Ptr &jsModule){
    m_d->jsModule = jsModule;
}

void ModuleFile::initializeImportsExports(Engine *engine){
//    m_d->imports = new Imports(engine, this);
//    m_d->exports = new Object(engine);
//    *m_d->exports = Object::create(engine);
}

bool ModuleFile::hasDependency(ModuleFile *module, ModuleFile *dependency){
    for ( auto it = module->m_d->dependencies.begin(); it != module->m_d->dependencies.end(); ++it ){
        if ( *it == dependency )
            return true;
    }
    return false;
}

PackageGraph::CyclesResult<ModuleFile *> ModuleFile::checkCycles(ModuleFile *mf){
    std::list<ModuleFile*> path;
    path.push_back(mf);

    for ( auto it = mf->m_d->dependencies.begin(); it != mf->m_d->dependencies.end(); ++it ){
        PackageGraph::CyclesResult<ModuleFile*> cr = checkCycles(mf, *it, path);
        if ( cr.found() )
            return cr;
    }
    return PackageGraph::CyclesResult<ModuleFile*>(PackageGraph::CyclesResult<ModuleFile*>::NotFound);;
}

PackageGraph::CyclesResult<ModuleFile*> ModuleFile::checkCycles(
        ModuleFile *mf, ModuleFile *current, std::list<ModuleFile *> path)
{
    path.push_back(current);
    if ( current == mf )
        return PackageGraph::CyclesResult<ModuleFile*>(PackageGraph::CyclesResult<ModuleFile*>::Found, path);

    for ( auto it = current->m_d->dependencies.begin(); it != current->m_d->dependencies.end(); ++it ){
        PackageGraph::CyclesResult<ModuleFile*> cr = checkCycles(mf, *it, path);
        if ( cr.found() )
            return cr;
    }
    return PackageGraph::CyclesResult<ModuleFile*>(PackageGraph::CyclesResult<ModuleFile*>::NotFound);
}

ModuleFile::ModuleFile(ElementsModule::Ptr plugin, const std::string &name, const std::string &content, ProgramNode *node)
    : m_d(new ModuleFilePrivate)
{
    std::string componentName = name;
    size_t extension = componentName.find(".lv");
    if ( extension != std::string::npos )
        componentName = componentName.substr(0, extension);

    m_d->plugin = plugin;
    m_d->name = componentName;
    m_d->state = ModuleFile::Initiaized;
    m_d->importsObject = nullptr;
    m_d->content = content;
    m_d->rootNode = node;

    node->collectImportTypes(content);

    std::vector<BaseNode*> exports = node->exports();
    for ( auto val : exports ){
        if ( val->typeString() == "RootNewComponentExpression" ){
            auto expression = val->as<RootNewComponentExpressionNode>();
            ModuleFile::Export expt;
            expt.type = ModuleFile::Export::Element;
            expt.name = expression->name(content);
            m_d->exports.push_back(expt);

        } else if ( val->typeString() == "ComponentDeclaration"){
            auto expression = val->as<ComponentDeclarationNode>();
            ModuleFile::Export expt;
            expt.type = ModuleFile::Export::Component;
            expt.name = expression->name(content);
            m_d->exports.push_back(expt);
        }
    }

    std::vector<ImportNode*> imports = node->imports();
    for ( auto val : imports ){
        ModuleFile::Import imp;
        imp.uri = val->path(content);
        imp.as = val->as(content);
        imp.isRelative = val->isRelative();
        m_d->imports.push_back(imp);
    }
}

}} // namespace lv, el
