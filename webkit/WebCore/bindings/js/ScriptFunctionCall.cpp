

#include "config.h"
#include "ScriptFunctionCall.h"

#include "JSDOMBinding.h"
#include "ScriptString.h"
#include "ScriptValue.h"

#include <runtime/JSLock.h>
#include <runtime/UString.h>

using namespace JSC;

namespace WebCore {

ScriptFunctionCall::ScriptFunctionCall(const ScriptObject& thisObject, const String& name)
    : m_exec(thisObject.scriptState())
    , m_thisObject(thisObject)
    , m_name(name)
{
}

void ScriptFunctionCall::appendArgument(const ScriptObject& argument)
{
    if (argument.scriptState() != m_exec) {
        ASSERT_NOT_REACHED();
        return;
    }
    m_arguments.append(argument.jsObject());
}

void ScriptFunctionCall::appendArgument(const ScriptString& argument)
{
    m_arguments.append(jsString(m_exec, argument));
}

void ScriptFunctionCall::appendArgument(const ScriptValue& argument)
{
    m_arguments.append(argument.jsValue());
}

void ScriptFunctionCall::appendArgument(const String& argument)
{
    JSLock lock(SilenceAssertionsOnly);
    m_arguments.append(jsString(m_exec, argument));
}

void ScriptFunctionCall::appendArgument(const JSC::UString& argument)
{
    JSLock lock(SilenceAssertionsOnly);
    m_arguments.append(jsString(m_exec, argument));
}

void ScriptFunctionCall::appendArgument(const char* argument)
{
    JSLock lock(SilenceAssertionsOnly);
    m_arguments.append(jsString(m_exec, UString(argument)));
}

void ScriptFunctionCall::appendArgument(JSC::JSValue argument)
{
    m_arguments.append(argument);
}

void ScriptFunctionCall::appendArgument(long argument)
{
    JSLock lock(SilenceAssertionsOnly);
    m_arguments.append(jsNumber(m_exec, argument));
}

void ScriptFunctionCall::appendArgument(long long argument)
{
    JSLock lock(SilenceAssertionsOnly);
    m_arguments.append(jsNumber(m_exec, argument));
}

void ScriptFunctionCall::appendArgument(unsigned int argument)
{
    JSLock lock(SilenceAssertionsOnly);
    m_arguments.append(jsNumber(m_exec, argument));
}

void ScriptFunctionCall::appendArgument(unsigned long argument)
{
    JSLock lock(SilenceAssertionsOnly);
    m_arguments.append(jsNumber(m_exec, argument));
}

void ScriptFunctionCall::appendArgument(int argument)
{
    JSLock lock(SilenceAssertionsOnly);
    m_arguments.append(jsNumber(m_exec, argument));
}

void ScriptFunctionCall::appendArgument(bool argument)
{
    m_arguments.append(jsBoolean(argument));
}

ScriptValue ScriptFunctionCall::call(bool& hadException, bool reportExceptions)
{
    JSObject* thisObject = m_thisObject.jsObject();

    JSLock lock(SilenceAssertionsOnly);

    JSValue function = thisObject->get(m_exec, Identifier(m_exec, m_name));
    if (m_exec->hadException()) {
        if (reportExceptions)
            reportException(m_exec, m_exec->exception());

        hadException = true;
        return ScriptValue();
    }

    CallData callData;
    CallType callType = function.getCallData(callData);
    if (callType == CallTypeNone)
        return ScriptValue();

    JSValue result = JSC::call(m_exec, function, callType, callData, thisObject, m_arguments);
    if (m_exec->hadException()) {
        if (reportExceptions)
            reportException(m_exec, m_exec->exception());

        hadException = true;
        return ScriptValue();
    }

    return ScriptValue(result);
}

ScriptValue ScriptFunctionCall::call()
{
    bool hadException = false;
    return call(hadException);
}

ScriptObject ScriptFunctionCall::construct(bool& hadException, bool reportExceptions)
{
    JSObject* thisObject = m_thisObject.jsObject();

    JSLock lock(SilenceAssertionsOnly);

    JSObject* constructor = asObject(thisObject->get(m_exec, Identifier(m_exec, m_name)));
    if (m_exec->hadException()) {
        if (reportExceptions)
            reportException(m_exec, m_exec->exception());

        hadException = true;
        return ScriptObject();
    }

    ConstructData constructData;
    ConstructType constructType = constructor->getConstructData(constructData);
    if (constructType == ConstructTypeNone)
        return ScriptObject();

    JSValue result = JSC::construct(m_exec, constructor, constructType, constructData, m_arguments);
    if (m_exec->hadException()) {
        if (reportExceptions)
            reportException(m_exec, m_exec->exception());

        hadException = true;
        return ScriptObject();
    }

    return ScriptObject(m_exec, asObject(result));
}

} // namespace WebCore
