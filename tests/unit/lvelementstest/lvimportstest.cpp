#include "lvimportstest.h"
#include "live/elements/engine.h"
#include "live/applicationcontext.h"
#include "live/exception.h"
#include "live/elements/component.h"
#include "live/elements/element.h"

#include "testpack.h"
Q_TEST_RUNNER_REGISTER(LvImportsTest);

using namespace lv;
using namespace lv::el;

LvImportsTest::LvImportsTest(QObject *parent)
    : QObject(parent)
{
}

void LvImportsTest::initTestCase(){
}

std::string LvImportsTest::scriptPath(const std::string &scriptName){
    return lv::ApplicationContext::instance().releasePath() + "/data/" + scriptName;
}

std::string LvImportsTest::testPath(){
    return (QCoreApplication::applicationDirPath() + "/test").toStdString();
}

////////////////////////////////////////////////////

void LvImportsTest::singlePluginImportTest(){
    Engine* engine = new Engine;
    engine->setModuleFileType(Engine::Lv);

    TestPack tp(testPath(), true);
    tp.unpack(scriptPath("ImportTest01.lvep"));

    engine->scope([engine, &tp](){
        Element* main = engine->runFile(tp.path() + "/main.lv");
        QVERIFY(main != nullptr);
        QVERIFY(main->get("a").toStdString(engine) == "class[A]");
        QVERIFY(main->get("b").toStdString(engine) == "class[B]");
    });

    delete engine;
}

void LvImportsTest::samePathImportTest()
{
    Engine* engine = new Engine;
    engine->setModuleFileType(Engine::Lv);

    TestPack tp(testPath(), true);
    tp.unpack(scriptPath("ImportTest02.lvep"));

    engine->scope([engine, &tp](){
        Element* main = engine->runFile(tp.path() + "/main.lv");
        QVERIFY(main != nullptr);
        QVERIFY(main->get("a").toStdString(engine) == "class[A]");
        QVERIFY(main->get("b").toStdString(engine) == "class[B]");
    });

    delete engine;
}


void LvImportsTest::importPluginWithSamePathImportTest()
{
    Engine* engine = new Engine;
    engine->setModuleFileType(Engine::Lv);

    TestPack tp(testPath(), true);
    tp.unpack(scriptPath("ImportTest03.lvep"));

    engine->scope([engine, &tp](){
        Element* main = engine->runFile(tp.path() + "/main.lv");
        QVERIFY(main != nullptr);
        QVERIFY(main->get("b").toStdString(engine) == "class[A]");
    });

    delete engine;
}

void LvImportsTest::importPluginThatImportsPlugin()
{
    Engine* engine = new Engine;
    engine->setModuleFileType(Engine::Lv);

    TestPack tp(testPath(), true);
    tp.unpack(scriptPath("ImportTest04.lvep"));

    engine->scope([engine, &tp](){
        Element* main = engine->runFile(tp.path() + "/main.lv");
        QVERIFY(main != nullptr);
        QVERIFY(main->get("c").toStdString(engine) == "class[B]");
    });

    delete engine;
}

void LvImportsTest::samePathSingletonTest()
{
    Engine* engine = new Engine;
    engine->setModuleFileType(Engine::Lv);

    TestPack tp(testPath(), true);
    tp.unpack(scriptPath("ImportTest05.lvep"));

    engine->scope([engine, &tp](){
        Element* main = engine->runFile(tp.path() + "/main.lv");
        QVERIFY(main != nullptr);
        QVERIFY(main->get("a").toStdString(engine) == "class[A]");
        QVERIFY(main->get("b").toStdString(engine) == "class[B]");
    });

    delete engine;
}

void LvImportsTest::sameModuleDifferentNamespacesTest()
{
    Engine* engine = new Engine;
    engine->setModuleFileType(Engine::Lv);

    TestPack tp(testPath(), true);
    tp.unpack(scriptPath("ImportTest06.lvep"));

    engine->scope([engine, &tp](){
        Element* main = engine->runFile(tp.path() + "/main.lv");
        QVERIFY(main != nullptr);
        QVERIFY(main->get("a").toStdString(engine) == "class[A]");
        QVERIFY(main->get("b").toStdString(engine) == "class[B]");
        QVERIFY(main->get("c").toStdString(engine) == "class[C]");
    });

    delete engine;
}

void LvImportsTest::moduleFileDependencyCycleTest()
{
    Engine* engine = new Engine;
    engine->setModuleFileType(Engine::Lv);

    TestPack tp(testPath(), true);
    tp.unpack(scriptPath("ImportTest07.lvep"));

    engine->scope([engine, &tp](){
        bool hadException = false;
        try {
            engine->runFile(tp.path() + "/main.lv");
        } catch (lv::Exception& e) {
            QVERIFY(e.code() == lv::Exception::toCode("Cycle"));
            hadException = true;
        }
    });

    delete engine;
}

void LvImportsTest::pluginDependencyCycleTest(){

    Engine* engine = new Engine;
    engine->setModuleFileType(Engine::Lv);

    TestPack tp(testPath(), true);
    tp.unpack(scriptPath("ImportTest08.lvep"));

    engine->scope([engine, &tp](){
        bool hadException = false;
        try {
            engine->runFile(tp.path() + "/main.lv");
        } catch (lv::Exception& e) {
            QVERIFY(e.code() == lv::Exception::toCode("Cycle"));
            hadException = true;
        }
        QVERIFY(hadException);
    });

    delete engine;
}

void LvImportsTest::packageImportTest()
{
    Engine* engine = new Engine;
    engine->setModuleFileType(Engine::Lv);

    TestPack tp(testPath(), true);
    tp.unpack(scriptPath("ImportTest09.lvep"));

    engine->scope([engine, &tp](){
        engine->setPackageImportPaths({tp.path() + "/packages"});
        Element* main = engine->runFile(tp.path() + "/main.lv");
        QVERIFY(main != nullptr);
        QVERIFY(main->get("a").toStdString(engine) == "class[A]");
    });

    delete engine;
}


void LvImportsTest::packageDependencyCycleTest(){
    Engine* engine = new Engine;
    engine->setModuleFileType(Engine::Lv);

    TestPack tp(testPath(), true);
    tp.unpack(scriptPath("ImportTest10.lvep"));

    engine->scope([engine, &tp](){
        engine->setPackageImportPaths({tp.path() + "/packages"});
        bool hadException = false;
        try {
            engine->runFile(tp.path() + "/main.lv");
        } catch (lv::Exception& e) {
            QVERIFY(e.code() == lv::Exception::toCode("Cycle"));
            hadException = true;
        }
        QVERIFY(hadException);
    });

    delete engine;
}

void LvImportsTest::importAsTest()
{
    Engine* engine = new Engine;
    engine->setModuleFileType(Engine::Lv);

    TestPack tp(testPath(), true);
    tp.unpack(scriptPath("ImportTest11.lvep"));

    engine->scope([engine, &tp](){
        Element* main = engine->runFile(tp.path() + "/main.lv");
        QVERIFY(main != nullptr);
        QVERIFY(main->get("a").toStdString(engine) == "class[A]");
        QVERIFY(main->get("b").toStdString(engine) == "class[B]");
    });

    delete engine;
}

void LvImportsTest::importMetaTest(){
    Engine* engine = new Engine;
    engine->setModuleFileType(Engine::Lv);

    TestPack tp(testPath(), true);
    tp.unpack(scriptPath("ImportTest12.lvep"));

    engine->scope([engine, &tp](){
        Element* main = engine->runFile(tp.path() + "/main.lv");
        QVERIFY(main != nullptr);
        QVERIFY(main->get("aUrl").toStdString(engine) == tp.path() + "/plugin1/A.lv");
        QVERIFY(main->get("aModule").toStdString(engine) == "main.plugin1");
        QVERIFY(main->get("bUrl").toStdString(engine) == tp.path() + "/plugin1/subplugin/B.lv");
        QVERIFY(main->get("bModule").toStdString(engine) == "main.plugin1.subplugin");
    });

    delete engine;
}

