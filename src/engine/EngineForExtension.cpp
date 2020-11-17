// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.

#include "precomp.h"


static HRESULT CopyStringToBE(
    __in LPWSTR wzValue,
    __in LPWSTR wzBuffer,
    __inout DWORD* pcchBuffer
    );

static HRESULT BEEngineEscapeString(
    __in BURN_EXTENSION_ENGINE_CONTEXT* /*pContext*/,
    __in const LPVOID pvArgs,
    __inout LPVOID pvResults
    )
{
    HRESULT hr = S_OK;
    LPWSTR sczValue = NULL;
    ValidateMessageArgs(hr, pvArgs, BUNDLE_EXTENSION_ENGINE_ESCAPESTRING_ARGS, pArgs);
    ValidateMessageResults(hr, pvResults, BUNDLE_EXTENSION_ENGINE_ESCAPESTRING_RESULTS, pResults);
    LPCWSTR wzIn = pArgs->wzIn;
    LPWSTR wzOut = pResults->wzOut;
    DWORD* pcchOut = &pResults->cchOut;

    if (wzIn && *wzIn)
    {
        hr = VariableEscapeString(wzIn, &sczValue);
        if (SUCCEEDED(hr))
        {
            hr = CopyStringToBE(sczValue, wzOut, pcchOut);
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

LExit:
    StrSecureZeroFreeString(sczValue);
    return hr;
}

static HRESULT BEEngineEvaluateCondition(
    __in BURN_EXTENSION_ENGINE_CONTEXT* pContext,
    __in const LPVOID pvArgs,
    __inout LPVOID pvResults
    )
{
    HRESULT hr = S_OK;
    ValidateMessageArgs(hr, pvArgs, BUNDLE_EXTENSION_ENGINE_EVALUATECONDITION_ARGS, pArgs);
    ValidateMessageResults(hr, pvResults, BUNDLE_EXTENSION_ENGINE_EVALUATECONDITION_RESULTS, pResults);
    LPCWSTR wzCondition = pArgs->wzCondition;
    BOOL* pf = &pResults->f;

    if (wzCondition && *wzCondition)
    {
        hr = ConditionEvaluate(&pContext->pEngineState->variables, wzCondition, pf);
    }
    else
    {
        hr = E_INVALIDARG;
    }

LExit:
    return hr;
}

static HRESULT BEEngineFormatString(
    __in BURN_EXTENSION_ENGINE_CONTEXT* pContext,
    __in const LPVOID pvArgs,
    __inout LPVOID pvResults
    )
{
    HRESULT hr = S_OK;
    LPWSTR sczValue = NULL;
    ValidateMessageArgs(hr, pvArgs, BUNDLE_EXTENSION_ENGINE_FORMATSTRING_ARGS, pArgs);
    ValidateMessageResults(hr, pvResults, BUNDLE_EXTENSION_ENGINE_FORMATSTRING_RESULTS, pResults);
    LPCWSTR wzIn = pArgs->wzIn;
    LPWSTR wzOut = pResults->wzOut;
    DWORD* pcchOut = &pResults->cchOut;

    if (wzIn && *wzIn)
    {
        hr = VariableFormatString(&pContext->pEngineState->variables, wzIn, &sczValue, NULL);
        if (SUCCEEDED(hr))
        {
            hr = CopyStringToBE(sczValue, wzOut, pcchOut);
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

LExit:
    StrSecureZeroFreeString(sczValue);
    return hr;
}

static HRESULT BEEngineGetVariableNumeric(
    __in BURN_EXTENSION_ENGINE_CONTEXT* pContext,
    __in const LPVOID pvArgs,
    __inout LPVOID pvResults
    )
{
    HRESULT hr = S_OK;
    ValidateMessageArgs(hr, pvArgs, BUNDLE_EXTENSION_ENGINE_GETVARIABLENUMERIC_ARGS, pArgs);
    ValidateMessageResults(hr, pvResults, BUNDLE_EXTENSION_ENGINE_GETVARIABLENUMERIC_RESULTS, pResults);
    LPCWSTR wzVariable = pArgs->wzVariable;
    LONGLONG* pllValue = &pResults->llValue;

    if (wzVariable && *wzVariable)
    {
        hr = VariableGetNumeric(&pContext->pEngineState->variables, wzVariable, pllValue);
    }
    else
    {
        hr = E_INVALIDARG;
    }

LExit:
    return hr;
}

static HRESULT BEEngineGetVariableString(
    __in BURN_EXTENSION_ENGINE_CONTEXT* pContext,
    __in const LPVOID pvArgs,
    __inout LPVOID pvResults
    )
{
    HRESULT hr = S_OK;
    LPWSTR sczValue = NULL;
    ValidateMessageArgs(hr, pvArgs, BUNDLE_EXTENSION_ENGINE_GETVARIABLESTRING_ARGS, pArgs);
    ValidateMessageResults(hr, pvResults, BUNDLE_EXTENSION_ENGINE_GETVARIABLESTRING_RESULTS, pResults);
    LPCWSTR wzVariable = pArgs->wzVariable;
    LPWSTR wzValue = pResults->wzValue;
    DWORD* pcchValue = &pResults->cchValue;

    if (wzVariable && *wzVariable)
    {
        hr = VariableGetString(&pContext->pEngineState->variables, wzVariable, &sczValue);
        if (SUCCEEDED(hr))
        {
            hr = CopyStringToBE(sczValue, wzValue, pcchValue);
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

LExit:
    StrSecureZeroFreeString(sczValue);
    return hr;
}

static HRESULT BEEngineGetVariableVersion(
    __in BURN_EXTENSION_ENGINE_CONTEXT* pContext,
    __in const LPVOID pvArgs,
    __inout LPVOID pvResults
    )
{
    HRESULT hr = S_OK;
    VERUTIL_VERSION* pVersion = NULL;
    ValidateMessageArgs(hr, pvArgs, BUNDLE_EXTENSION_ENGINE_GETVARIABLEVERSION_ARGS, pArgs);
    ValidateMessageResults(hr, pvResults, BUNDLE_EXTENSION_ENGINE_GETVARIABLEVERSION_RESULTS, pResults);
    LPCWSTR wzVariable = pArgs->wzVariable;
    LPWSTR wzValue = pResults->wzValue;
    DWORD* pcchValue = &pResults->cchValue;

    if (wzVariable && *wzVariable)
    {
        hr = VariableGetVersion(&pContext->pEngineState->variables, wzVariable, &pVersion);
        if (SUCCEEDED(hr))
        {
            hr = CopyStringToBE(pVersion->sczVersion, wzValue, pcchValue);
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

LExit:
    ReleaseVerutilVersion(pVersion);

    return hr;
}

static HRESULT BEEngineLog(
    __in BURN_EXTENSION_ENGINE_CONTEXT* /*pContext*/,
    __in const LPVOID pvArgs,
    __inout LPVOID pvResults
    )
{
    HRESULT hr = S_OK;
    REPORT_LEVEL rl = REPORT_NONE;
    ValidateMessageArgs(hr, pvArgs, BUNDLE_EXTENSION_ENGINE_LOG_ARGS, pArgs);
    ValidateMessageResults(hr, pvResults, BUNDLE_EXTENSION_ENGINE_LOG_RESULTS, pResults);
    BUNDLE_EXTENSION_LOG_LEVEL level = pArgs->level;
    LPCWSTR wzMessage = pArgs->wzMessage;

    switch (level)
    {
    case BUNDLE_EXTENSION_LOG_LEVEL_STANDARD:
        rl = REPORT_STANDARD;
        break;

    case BUNDLE_EXTENSION_LOG_LEVEL_VERBOSE:
        rl = REPORT_VERBOSE;
        break;

    case BUNDLE_EXTENSION_LOG_LEVEL_DEBUG:
        rl = REPORT_DEBUG;
        break;

    case BUNDLE_EXTENSION_LOG_LEVEL_ERROR:
        rl = REPORT_ERROR;
        break;

    default:
        ExitFunction1(hr = E_INVALIDARG);
    }

    hr = LogStringLine(rl, "%ls", wzMessage);
    ExitOnFailure(hr, "Failed to log Bundle Extension message.");

LExit:
    return hr;
}

static HRESULT BEEngineSetVariableNumeric(
    __in BURN_EXTENSION_ENGINE_CONTEXT* pContext,
    __in const LPVOID pvArgs,
    __inout LPVOID pvResults
    )
{
    HRESULT hr = S_OK;
    ValidateMessageArgs(hr, pvArgs, BUNDLE_EXTENSION_ENGINE_SETVARIABLENUMERIC_ARGS, pArgs);
    ValidateMessageResults(hr, pvResults, BUNDLE_EXTENSION_ENGINE_SETVARIABLENUMERIC_RESULTS, pResults);
    LPCWSTR wzVariable = pArgs->wzVariable;
    LONGLONG llValue = pArgs->llValue;

    if (wzVariable && *wzVariable)
    {
        hr = VariableSetNumeric(&pContext->pEngineState->variables, wzVariable, llValue, FALSE);
        ExitOnFailure(hr, "Failed to set numeric variable.");
    }
    else
    {
        hr = E_INVALIDARG;
        ExitOnFailure(hr, "Bundle Extension did not provide variable name.");
    }

LExit:
    return hr;
}

static HRESULT BEEngineSetVariableString(
    __in BURN_EXTENSION_ENGINE_CONTEXT* pContext,
    __in const LPVOID pvArgs,
    __inout LPVOID pvResults
    )
{
    HRESULT hr = S_OK;
    ValidateMessageArgs(hr, pvArgs, BUNDLE_EXTENSION_ENGINE_SETVARIABLESTRING_ARGS, pArgs);
    ValidateMessageResults(hr, pvResults, BUNDLE_EXTENSION_ENGINE_SETVARIABLESTRING_RESULTS, pResults);
    LPCWSTR wzVariable = pArgs->wzVariable;
    LPCWSTR wzValue = pArgs->wzValue;

    if (wzVariable && *wzVariable)
    {
        hr = VariableSetString(&pContext->pEngineState->variables, wzVariable, wzValue, FALSE, pArgs->fFormatted);
        ExitOnFailure(hr, "Failed to set string variable.");
    }
    else
    {
        hr = E_INVALIDARG;
        ExitOnFailure(hr, "Bundle Extension did not provide variable name.");
    }

LExit:
    return hr;
}

static HRESULT BEEngineSetVariableVersion(
    __in BURN_EXTENSION_ENGINE_CONTEXT* pContext,
    __in const LPVOID pvArgs,
    __inout LPVOID pvResults
    )
{
    HRESULT hr = S_OK;
    VERUTIL_VERSION* pVersion = NULL;
    ValidateMessageArgs(hr, pvArgs, BUNDLE_EXTENSION_ENGINE_SETVARIABLEVERSION_ARGS, pArgs);
    ValidateMessageResults(hr, pvResults, BUNDLE_EXTENSION_ENGINE_SETVARIABLEVERSION_RESULTS, pResults);
    LPCWSTR wzVariable = pArgs->wzVariable;
    LPCWSTR wzValue = pArgs->wzValue;

    if (wzVariable && *wzVariable)
    {
        if (wzValue)
        {
            hr = VerParseVersion(wzValue, 0, FALSE, &pVersion);
            ExitOnFailure(hr, "Failed to parse new version value.");
        }

        hr = VariableSetVersion(&pContext->pEngineState->variables, wzVariable, pVersion, FALSE);
        ExitOnFailure(hr, "Failed to set version variable.");
    }
    else
    {
        hr = E_INVALIDARG;
        ExitOnFailure(hr, "Bundle Extension did not provide variable name.");
    }

LExit:
    ReleaseVerutilVersion(pVersion);

    return hr;
}

static HRESULT BEEngineCompareVersions(
    __in BURN_EXTENSION_ENGINE_CONTEXT* /*pContext*/,
    __in const LPVOID pvArgs,
    __inout LPVOID pvResults
    )
{
    HRESULT hr = S_OK;
    ValidateMessageArgs(hr, pvArgs, BUNDLE_EXTENSION_ENGINE_COMPAREVERSIONS_ARGS, pArgs);
    ValidateMessageResults(hr, pvResults, BUNDLE_EXTENSION_ENGINE_COMPAREVERSIONS_RESULTS, pResults);
    LPCWSTR wzVersion1 = pArgs->wzVersion1;
    LPCWSTR wzVersion2 = pArgs->wzVersion2;
    int* pnResult = &pResults->nResult;

    hr = VerCompareStringVersions(wzVersion1, wzVersion2, FALSE, pnResult);

LExit:
    return hr;
}

HRESULT WINAPI EngineForExtensionProc(
    __in BUNDLE_EXTENSION_ENGINE_MESSAGE message,
    __in const LPVOID pvArgs,
    __inout LPVOID pvResults,
    __in_opt LPVOID pvContext
    )
{
    HRESULT hr = S_OK;
    BURN_EXTENSION_ENGINE_CONTEXT* pContext = reinterpret_cast<BURN_EXTENSION_ENGINE_CONTEXT*>(pvContext);

    if (!pContext || !pvArgs || !pvResults)
    {
        ExitFunction1(hr = E_INVALIDARG);
    }

    switch (message)
    {
    case BUNDLE_EXTENSION_ENGINE_MESSAGE_ESCAPESTRING:
        hr = BEEngineEscapeString(pContext, pvArgs, pvResults);
        break;
    case BUNDLE_EXTENSION_ENGINE_MESSAGE_EVALUATECONDITION:
        hr = BEEngineEvaluateCondition(pContext, pvArgs, pvResults);
        break;
    case BUNDLE_EXTENSION_ENGINE_MESSAGE_FORMATSTRING:
        hr = BEEngineFormatString(pContext, pvArgs, pvResults);
        break;
    case BUNDLE_EXTENSION_ENGINE_MESSAGE_GETVARIABLENUMERIC:
        hr = BEEngineGetVariableNumeric(pContext, pvArgs, pvResults);
        break;
    case BUNDLE_EXTENSION_ENGINE_MESSAGE_GETVARIABLESTRING:
        hr = BEEngineGetVariableString(pContext, pvArgs, pvResults);
        break;
    case BUNDLE_EXTENSION_ENGINE_MESSAGE_GETVARIABLEVERSION:
        hr = BEEngineGetVariableVersion(pContext, pvArgs, pvResults);
        break;
    case BUNDLE_EXTENSION_ENGINE_MESSAGE_LOG:
        hr = BEEngineLog(pContext, pvArgs, pvResults);
        break;
    case BUNDLE_EXTENSION_ENGINE_MESSAGE_SETVARIABLENUMERIC:
        hr = BEEngineSetVariableNumeric(pContext, pvArgs, pvResults);
        break;
    case BUNDLE_EXTENSION_ENGINE_MESSAGE_SETVARIABLESTRING:
        hr = BEEngineSetVariableString(pContext, pvArgs, pvResults);
        break;
    case BUNDLE_EXTENSION_ENGINE_MESSAGE_SETVARIABLEVERSION:
        hr = BEEngineSetVariableVersion(pContext, pvArgs, pvResults);
        break;
    case BUNDLE_EXTENSION_ENGINE_MESSAGE_COMPAREVERSIONS:
        hr = BEEngineCompareVersions(pContext, pvArgs, pvResults);
        break;
    default:
        hr = E_NOTIMPL;
        break;
    }

LExit:
    return hr;
}

static HRESULT CopyStringToBE(
    __in LPWSTR wzValue,
    __in LPWSTR wzBuffer,
    __inout DWORD* pcchBuffer
    )
{
    HRESULT hr = S_OK;
    BOOL fTooSmall = !wzBuffer;

    if (!fTooSmall)
    {
        hr = ::StringCchCopyExW(wzBuffer, *pcchBuffer, wzValue, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
        if (STRSAFE_E_INSUFFICIENT_BUFFER == hr)
        {
            fTooSmall = TRUE;
        }
    }

    if (fTooSmall)
    {
        hr = ::StringCchLengthW(wzValue, STRSAFE_MAX_CCH, reinterpret_cast<size_t*>(pcchBuffer));
        if (SUCCEEDED(hr))
        {
            hr = E_MOREDATA;
            *pcchBuffer += 1; // null terminator.
        }
    }

    return hr;
}
