// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.

#include "precomp.h"


// constants


// structs



// internal function declarations

static HRESULT ParseRelatedMsiFromXml(
    __in IXMLDOMNode* pixnRelatedMsi,
    __in BURN_RELATED_MSI* pRelatedMsi
    );
static HRESULT EvaluateActionStateConditions(
    __in BURN_VARIABLES* pVariables,
    __in_z_opt LPCWSTR sczAddLocalCondition,
    __in_z_opt LPCWSTR sczAddSourceCondition,
    __in_z_opt LPCWSTR sczAdvertiseCondition,
    __out BOOTSTRAPPER_FEATURE_STATE* pState
    );
static HRESULT CalculateFeatureAction(
    __in BOOTSTRAPPER_FEATURE_STATE currentState,
    __in BOOTSTRAPPER_FEATURE_STATE requestedState,
    __in BOOL fRepair,
    __out BOOTSTRAPPER_FEATURE_ACTION* pFeatureAction,
    __inout BOOL* pfDelta
    );
static HRESULT EscapePropertyArgumentString(
    __in LPCWSTR wzProperty,
    __inout_z LPWSTR* psczEscapedValue,
    __in BOOL fZeroOnRealloc
    );
static HRESULT ConcatFeatureActionProperties(
    __in BURN_PACKAGE* pPackage,
    __in BOOTSTRAPPER_FEATURE_ACTION* rgFeatureActions,
    __inout_z LPWSTR* psczArguments
    );
static HRESULT ConcatPatchProperty(
    __in BURN_PACKAGE* pPackage,
    __in BOOL fRollback,
    __inout_z LPWSTR* psczArguments
    );
static void RegisterSourceDirectory(
    __in BURN_PACKAGE* pPackage,
    __in_z LPCWSTR wzCacheDirectory
    );


// function definitions

extern "C" HRESULT MsiEngineParsePackageFromXml(
    __in IXMLDOMNode* pixnMsiPackage,
    __in BURN_PACKAGE* pPackage
    )
{
    HRESULT hr = S_OK;
    IXMLDOMNodeList* pixnNodes = NULL;
    IXMLDOMNode* pixnNode = NULL;
    DWORD cNodes = 0;
    LPWSTR scz = NULL;

    // @ProductCode
    hr = XmlGetAttributeEx(pixnMsiPackage, L"ProductCode", &pPackage->Msi.sczProductCode);
    ExitOnFailure(hr, "Failed to get @ProductCode.");

    // @Language
    hr = XmlGetAttributeNumber(pixnMsiPackage, L"Language", &pPackage->Msi.dwLanguage);
    ExitOnFailure(hr, "Failed to get @Language.");

    // @Version
    hr = XmlGetAttributeEx(pixnMsiPackage, L"Version", &scz);
    ExitOnFailure(hr, "Failed to get @Version.");

    hr = VerParseVersion(scz, 0, FALSE, &pPackage->Msi.pVersion);
    ExitOnFailure(hr, "Failed to parse @Version: %ls", scz);

    if (pPackage->Msi.pVersion->fInvalid)
    {
        LogId(REPORT_WARNING, MSG_MANIFEST_INVALID_VERSION, scz);
    }

    // @UpgradeCode
    hr = XmlGetAttributeEx(pixnMsiPackage, L"UpgradeCode", &pPackage->Msi.sczUpgradeCode);
    if (E_NOTFOUND != hr)
    {
        ExitOnFailure(hr, "Failed to get @UpgradeCode.");
    }

    // select feature nodes
    hr = XmlSelectNodes(pixnMsiPackage, L"MsiFeature", &pixnNodes);
    ExitOnFailure(hr, "Failed to select feature nodes.");

    // get feature node count
    hr = pixnNodes->get_length((long*)&cNodes);
    ExitOnFailure(hr, "Failed to get feature node count.");

    if (cNodes)
    {
        // allocate memory for features
        pPackage->Msi.rgFeatures = (BURN_MSIFEATURE*)MemAlloc(sizeof(BURN_MSIFEATURE) * cNodes, TRUE);
        ExitOnNull(pPackage->Msi.rgFeatures, hr, E_OUTOFMEMORY, "Failed to allocate memory for MSI feature structs.");

        pPackage->Msi.cFeatures = cNodes;

        // parse feature elements
        for (DWORD i = 0; i < cNodes; ++i)
        {
            BURN_MSIFEATURE* pFeature = &pPackage->Msi.rgFeatures[i];

            hr = XmlNextElement(pixnNodes, &pixnNode, NULL);
            ExitOnFailure(hr, "Failed to get next node.");

            // @Id
            hr = XmlGetAttributeEx(pixnNode, L"Id", &pFeature->sczId);
            ExitOnFailure(hr, "Failed to get @Id.");

            // @AddLocalCondition
            hr = XmlGetAttributeEx(pixnNode, L"AddLocalCondition", &pFeature->sczAddLocalCondition);
            if (E_NOTFOUND != hr)
            {
                ExitOnFailure(hr, "Failed to get @AddLocalCondition.");
            }

            // @AddSourceCondition
            hr = XmlGetAttributeEx(pixnNode, L"AddSourceCondition", &pFeature->sczAddSourceCondition);
            if (E_NOTFOUND != hr)
            {
                ExitOnFailure(hr, "Failed to get @AddSourceCondition.");
            }

            // @AdvertiseCondition
            hr = XmlGetAttributeEx(pixnNode, L"AdvertiseCondition", &pFeature->sczAdvertiseCondition);
            if (E_NOTFOUND != hr)
            {
                ExitOnFailure(hr, "Failed to get @AdvertiseCondition.");
            }

            // @RollbackAddLocalCondition
            hr = XmlGetAttributeEx(pixnNode, L"RollbackAddLocalCondition", &pFeature->sczRollbackAddLocalCondition);
            if (E_NOTFOUND != hr)
            {
                ExitOnFailure(hr, "Failed to get @RollbackAddLocalCondition.");
            }

            // @RollbackAddSourceCondition
            hr = XmlGetAttributeEx(pixnNode, L"RollbackAddSourceCondition", &pFeature->sczRollbackAddSourceCondition);
            if (E_NOTFOUND != hr)
            {
                ExitOnFailure(hr, "Failed to get @RollbackAddSourceCondition.");
            }

            // @RollbackAdvertiseCondition
            hr = XmlGetAttributeEx(pixnNode, L"RollbackAdvertiseCondition", &pFeature->sczRollbackAdvertiseCondition);
            if (E_NOTFOUND != hr)
            {
                ExitOnFailure(hr, "Failed to get @RollbackAdvertiseCondition.");
            }

            // prepare next iteration
            ReleaseNullObject(pixnNode);
        }
    }

    ReleaseNullObject(pixnNodes); // done with the MsiFeature elements.

    hr = MsiEngineParsePropertiesFromXml(pixnMsiPackage, &pPackage->Msi.rgProperties, &pPackage->Msi.cProperties);
    ExitOnFailure(hr, "Failed to parse properties from XML.");

    // select related MSI nodes
    hr = XmlSelectNodes(pixnMsiPackage, L"RelatedPackage", &pixnNodes);
    ExitOnFailure(hr, "Failed to select related MSI nodes.");

    // get related MSI node count
    hr = pixnNodes->get_length((long*)&cNodes);
    ExitOnFailure(hr, "Failed to get related MSI node count.");

    if (cNodes)
    {
        // allocate memory for related MSIs
        pPackage->Msi.rgRelatedMsis = (BURN_RELATED_MSI*)MemAlloc(sizeof(BURN_RELATED_MSI) * cNodes, TRUE);
        ExitOnNull(pPackage->Msi.rgRelatedMsis, hr, E_OUTOFMEMORY, "Failed to allocate memory for related MSI structs.");

        pPackage->Msi.cRelatedMsis = cNodes;

        // parse related MSI elements
        for (DWORD i = 0; i < cNodes; ++i)
        {
            hr = XmlNextElement(pixnNodes, &pixnNode, NULL);
            ExitOnFailure(hr, "Failed to get next node.");

            // parse related MSI element
            hr = ParseRelatedMsiFromXml(pixnNode, &pPackage->Msi.rgRelatedMsis[i]);
            ExitOnFailure(hr, "Failed to parse related MSI element.");

            // prepare next iteration
            ReleaseNullObject(pixnNode);
        }
    }

    ReleaseNullObject(pixnNodes); // done with the RelatedPackage elements.

    // Select slipstream MSP nodes.
    hr = XmlSelectNodes(pixnMsiPackage, L"SlipstreamMsp", &pixnNodes);
    ExitOnFailure(hr, "Failed to select related MSI nodes.");

    hr = pixnNodes->get_length((long*)&cNodes);
    ExitOnFailure(hr, "Failed to get related MSI node count.");

    if (cNodes)
    {
        pPackage->Msi.rgSlipstreamMsps = reinterpret_cast<BURN_SLIPSTREAM_MSP*>(MemAlloc(sizeof(BURN_SLIPSTREAM_MSP) * cNodes, TRUE));
        ExitOnNull(pPackage->Msi.rgSlipstreamMsps, hr, E_OUTOFMEMORY, "Failed to allocate memory for slipstream MSP packages.");

        pPackage->Msi.rgsczSlipstreamMspPackageIds = reinterpret_cast<LPWSTR*>(MemAlloc(sizeof(LPWSTR*) * cNodes, TRUE));
        ExitOnNull(pPackage->Msi.rgsczSlipstreamMspPackageIds, hr, E_OUTOFMEMORY, "Failed to allocate memory for slipstream MSP ids.");

        pPackage->Msi.cSlipstreamMspPackages = cNodes;

        // Parse slipstream MSP Ids.
        for (DWORD i = 0; i < cNodes; ++i)
        {
            hr = XmlNextElement(pixnNodes, &pixnNode, NULL);
            ExitOnFailure(hr, "Failed to get next slipstream MSP node.");

            hr = XmlGetAttributeEx(pixnNode, L"Id", pPackage->Msi.rgsczSlipstreamMspPackageIds + i);
            ExitOnFailure(hr, "Failed to parse slipstream MSP ids.");

            ReleaseNullObject(pixnNode);
        }
    }

    hr = S_OK;

LExit:
    ReleaseObject(pixnNodes);
    ReleaseObject(pixnNode);
    ReleaseStr(scz);

    return hr;
}

extern "C" HRESULT MsiEngineParsePropertiesFromXml(
    __in IXMLDOMNode* pixnPackage,
    __out BURN_MSIPROPERTY** prgProperties,
    __out DWORD* pcProperties
    )
{
    HRESULT hr = S_OK;
    IXMLDOMNodeList* pixnNodes = NULL;
    IXMLDOMNode* pixnNode = NULL;
    DWORD cNodes = 0;

    BURN_MSIPROPERTY* pProperties = NULL;

    // select property nodes
    hr = XmlSelectNodes(pixnPackage, L"MsiProperty", &pixnNodes);
    ExitOnFailure(hr, "Failed to select property nodes.");

    // get property node count
    hr = pixnNodes->get_length((long*)&cNodes);
    ExitOnFailure(hr, "Failed to get property node count.");

    if (cNodes)
    {
        // allocate memory for properties
        pProperties = (BURN_MSIPROPERTY*)MemAlloc(sizeof(BURN_MSIPROPERTY) * cNodes, TRUE);
        ExitOnNull(pProperties, hr, E_OUTOFMEMORY, "Failed to allocate memory for MSI property structs.");

        // parse property elements
        for (DWORD i = 0; i < cNodes; ++i)
        {
            BURN_MSIPROPERTY* pProperty = &pProperties[i];

            hr = XmlNextElement(pixnNodes, &pixnNode, NULL);
            ExitOnFailure(hr, "Failed to get next node.");

            // @Id
            hr = XmlGetAttributeEx(pixnNode, L"Id", &pProperty->sczId);
            ExitOnFailure(hr, "Failed to get @Id.");

            // @Value
            hr = XmlGetAttributeEx(pixnNode, L"Value", &pProperty->sczValue);
            ExitOnFailure(hr, "Failed to get @Value.");

            // @RollbackValue
            hr = XmlGetAttributeEx(pixnNode, L"RollbackValue", &pProperty->sczRollbackValue);
            if (E_NOTFOUND != hr)
            {
                ExitOnFailure(hr, "Failed to get @RollbackValue.");
            }

            // @Condition
            hr = XmlGetAttributeEx(pixnNode, L"Condition", &pProperty->sczCondition);
            if (E_NOTFOUND != hr)
            {
                ExitOnFailure(hr, "Failed to get @Condition.");
            }

            // prepare next iteration
            ReleaseNullObject(pixnNode);
        }
    }

    *pcProperties = cNodes;
    *prgProperties = pProperties;
    pProperties = NULL;

    hr = S_OK;

LExit:
    ReleaseNullObject(pixnNodes);
    ReleaseMem(pProperties);

    return hr;
}

extern "C" void MsiEnginePackageUninitialize(
    __in BURN_PACKAGE* pPackage
    )
{
    ReleaseStr(pPackage->Msi.sczProductCode);
    ReleaseStr(pPackage->Msi.sczUpgradeCode);

    // free features
    if (pPackage->Msi.rgFeatures)
    {
        for (DWORD i = 0; i < pPackage->Msi.cFeatures; ++i)
        {
            BURN_MSIFEATURE* pFeature = &pPackage->Msi.rgFeatures[i];

            ReleaseStr(pFeature->sczId);
            ReleaseStr(pFeature->sczAddLocalCondition);
            ReleaseStr(pFeature->sczAddSourceCondition);
            ReleaseStr(pFeature->sczAdvertiseCondition);
            ReleaseStr(pFeature->sczRollbackAddLocalCondition);
            ReleaseStr(pFeature->sczRollbackAddSourceCondition);
            ReleaseStr(pFeature->sczRollbackAdvertiseCondition);
        }
        MemFree(pPackage->Msi.rgFeatures);
    }

    // free properties
    if (pPackage->Msi.rgProperties)
    {
        for (DWORD i = 0; i < pPackage->Msi.cProperties; ++i)
        {
            BURN_MSIPROPERTY* pProperty = &pPackage->Msi.rgProperties[i];

            ReleaseStr(pProperty->sczId);
            ReleaseStr(pProperty->sczValue);
            ReleaseStr(pProperty->sczRollbackValue);
            ReleaseStr(pProperty->sczCondition);
        }
        MemFree(pPackage->Msi.rgProperties);
    }

    // free related MSIs
    if (pPackage->Msi.rgRelatedMsis)
    {
        for (DWORD i = 0; i < pPackage->Msi.cRelatedMsis; ++i)
        {
            BURN_RELATED_MSI* pRelatedMsi = &pPackage->Msi.rgRelatedMsis[i];

            ReleaseStr(pRelatedMsi->sczUpgradeCode);
            ReleaseMem(pRelatedMsi->rgdwLanguages);
        }
        MemFree(pPackage->Msi.rgRelatedMsis);
    }

    // free slipstream MSPs
    if (pPackage->Msi.rgsczSlipstreamMspPackageIds)
    {
        for (DWORD i = 0; i < pPackage->Msi.cSlipstreamMspPackages; ++i)
        {
            ReleaseStr(pPackage->Msi.rgsczSlipstreamMspPackageIds[i]);
        }

        MemFree(pPackage->Msi.rgsczSlipstreamMspPackageIds);
    }

    if (pPackage->Msi.rgSlipstreamMsps)
    {
        MemFree(pPackage->Msi.rgSlipstreamMsps);
    }

    if (pPackage->Msi.rgChainedPatches)
    {
        MemFree(pPackage->Msi.rgChainedPatches);
    }

    // clear struct
    memset(&pPackage->Msi, 0, sizeof(pPackage->Msi));
}

extern "C" HRESULT MsiEngineDetectInitialize(
    __in BURN_PACKAGES* pPackages
    )
{
    AssertSz(pPackages->cPatchInfo, "MsiEngineDetectInitialize() should only be called if there are MSP packages.");

    HRESULT hr = S_OK;

    // Add target products for slipstream MSIs that weren't detected.
    for (DWORD iPackage = 0; iPackage < pPackages->cPackages; ++iPackage)
    {
        BURN_PACKAGE* pMsiPackage = pPackages->rgPackages + iPackage;
        if (BURN_PACKAGE_TYPE_MSI == pMsiPackage->type)
        {
            for (DWORD j = 0; j < pMsiPackage->Msi.cSlipstreamMspPackages; ++j)
            {
                BURN_SLIPSTREAM_MSP* pSlipstreamMsp = pMsiPackage->Msi.rgSlipstreamMsps + j;
                Assert(pSlipstreamMsp->pMspPackage && BURN_PACKAGE_TYPE_MSP == pSlipstreamMsp->pMspPackage->type);

                if (pSlipstreamMsp->pMspPackage && BURN_PACKAGE_INVALID_PATCH_INDEX == pSlipstreamMsp->dwMsiChainedPatchIndex)
                {
                    hr = MspEngineAddMissingSlipstreamTarget(pMsiPackage, pSlipstreamMsp);
                    ExitOnFailure(hr, "Failed to add slipstreamed target product code to package: %ls", pSlipstreamMsp->pMspPackage->sczId);
                }
            }
        }
    }

LExit:
    return hr;
}

extern "C" HRESULT MsiEngineDetectPackage(
    __in BURN_PACKAGE* pPackage,
    __in BURN_USER_EXPERIENCE* pUserExperience
    )
{
    Trace(REPORT_STANDARD, "Detecting MSI package 0x%p", pPackage);

    HRESULT hr = S_OK;
    int nCompareResult = 0;
    LPWSTR sczInstalledVersion = NULL;
    LPWSTR sczInstalledLanguage = NULL;
    INSTALLSTATE installState = INSTALLSTATE_UNKNOWN;
    BOOTSTRAPPER_RELATED_OPERATION operation = BOOTSTRAPPER_RELATED_OPERATION_NONE;
    BOOTSTRAPPER_RELATED_OPERATION relatedMsiOperation = BOOTSTRAPPER_RELATED_OPERATION_NONE;
    WCHAR wzProductCode[MAX_GUID_CHARS + 1] = { };
    VERUTIL_VERSION* pVersion = NULL;
    UINT uLcid = 0;
    BOOL fPerMachine = FALSE;

    // detect self by product code
    // TODO: what to do about MSIINSTALLCONTEXT_USERMANAGED?
    hr = WiuGetProductInfoEx(pPackage->Msi.sczProductCode, NULL, pPackage->fPerMachine ? MSIINSTALLCONTEXT_MACHINE : MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_VERSIONSTRING, &sczInstalledVersion);
    if (SUCCEEDED(hr))
    {
        hr = VerParseVersion(sczInstalledVersion, 0, FALSE, &pPackage->Msi.pInstalledVersion);
        ExitOnFailure(hr, "Failed to parse installed version: '%ls' for ProductCode: %ls", sczInstalledVersion, pPackage->Msi.sczProductCode);

        if (pPackage->Msi.pInstalledVersion->fInvalid)
        {
            LogId(REPORT_WARNING, MSG_DETECTED_MSI_PACKAGE_INVALID_VERSION, pPackage->Msi.sczProductCode, sczInstalledVersion);
        }

        // compare versions
        hr = VerCompareParsedVersions(pPackage->Msi.pVersion, pPackage->Msi.pInstalledVersion, &nCompareResult);
        ExitOnFailure(hr, "Failed to compare version '%ls' to installed version: '%ls'", pPackage->Msi.pVersion->sczVersion, pPackage->Msi.pInstalledVersion->sczVersion);

        if (nCompareResult < 0)
        {
            operation = BOOTSTRAPPER_RELATED_OPERATION_DOWNGRADE;
            pPackage->currentState = BOOTSTRAPPER_PACKAGE_STATE_SUPERSEDED;
        }
        else
        {
            if (nCompareResult > 0)
            {
                operation = BOOTSTRAPPER_RELATED_OPERATION_MINOR_UPDATE;
            }

            pPackage->currentState = BOOTSTRAPPER_PACKAGE_STATE_PRESENT;
        }

        // Report related MSI package to BA.
        if (BOOTSTRAPPER_RELATED_OPERATION_NONE != operation)
        {
            LogId(REPORT_STANDARD, MSG_DETECTED_RELATED_PACKAGE, pPackage->Msi.sczProductCode, LoggingPerMachineToString(pPackage->fPerMachine), pPackage->Msi.pInstalledVersion->sczVersion, pPackage->Msi.dwLanguage, LoggingRelatedOperationToString(operation));

            hr = UserExperienceOnDetectRelatedMsiPackage(pUserExperience, pPackage->sczId, pPackage->Msi.sczUpgradeCode, pPackage->Msi.sczProductCode, pPackage->fPerMachine, pPackage->Msi.pInstalledVersion, operation);
            ExitOnRootFailure(hr, "BA aborted detect related MSI package.");
        }
    }
    else if (HRESULT_FROM_WIN32(ERROR_UNKNOWN_PRODUCT) == hr || HRESULT_FROM_WIN32(ERROR_UNKNOWN_PROPERTY) == hr) // package not present.
    {
        pPackage->currentState = BOOTSTRAPPER_PACKAGE_STATE_ABSENT;
        hr = S_OK;
    }
    else
    {
        ExitOnFailure(hr, "Failed to get product information for ProductCode: %ls", pPackage->Msi.sczProductCode);
    }

    // detect related packages by upgrade code
    for (DWORD i = 0; i < pPackage->Msi.cRelatedMsis; ++i)
    {
        BURN_RELATED_MSI* pRelatedMsi = &pPackage->Msi.rgRelatedMsis[i];

        for (DWORD iProduct = 0; ; ++iProduct)
        {
            // get product
            hr = WiuEnumRelatedProducts(pRelatedMsi->sczUpgradeCode, iProduct, wzProductCode);
            if (E_NOMOREITEMS == hr)
            {
                hr = S_OK;
                break;
            }
            ExitOnFailure(hr, "Failed to enum related products.");

            // If we found ourselves, skip because saying that a package is related to itself is nonsensical.
            if (CSTR_EQUAL == ::CompareStringW(LOCALE_NEUTRAL, NORM_IGNORECASE, pPackage->Msi.sczProductCode, -1, wzProductCode, -1))
            {
                continue;
            }

            // get product version
            hr = WiuGetProductInfoEx(wzProductCode, NULL, MSIINSTALLCONTEXT_MACHINE, INSTALLPROPERTY_VERSIONSTRING, &sczInstalledVersion);
            if (HRESULT_FROM_WIN32(ERROR_UNKNOWN_PRODUCT) != hr && HRESULT_FROM_WIN32(ERROR_UNKNOWN_PROPERTY) != hr)
            {
                ExitOnFailure(hr, "Failed to get version for product in machine context: %ls", wzProductCode);
                fPerMachine = TRUE;
            }
            else
            {
                hr = WiuGetProductInfoEx(wzProductCode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_VERSIONSTRING, &sczInstalledVersion);
                if (HRESULT_FROM_WIN32(ERROR_UNKNOWN_PRODUCT) != hr && HRESULT_FROM_WIN32(ERROR_UNKNOWN_PROPERTY) != hr)
                {
                    ExitOnFailure(hr, "Failed to get version for product in user unmanaged context: %ls", wzProductCode);
                    fPerMachine = FALSE;
                }
                else
                {
                    hr = S_OK;
                    continue;
                }
            }

            hr = VerParseVersion(sczInstalledVersion, 0, FALSE, &pVersion);
            ExitOnFailure(hr, "Failed to parse related installed version: '%ls' for ProductCode: %ls", sczInstalledVersion, wzProductCode);

            if (pVersion->fInvalid)
            {
                LogId(REPORT_WARNING, MSG_DETECTED_MSI_PACKAGE_INVALID_VERSION, wzProductCode, sczInstalledVersion);
            }

            // compare versions
            if (pRelatedMsi->fMinProvided)
            {
                hr = VerCompareParsedVersions(pVersion, pRelatedMsi->pMinVersion, &nCompareResult);
                ExitOnFailure(hr, "Failed to compare related installed version '%ls' to related min version: '%ls'", pVersion->sczVersion, pRelatedMsi->pMinVersion->sczVersion);

                if (pRelatedMsi->fMinInclusive ? (nCompareResult < 0) : (nCompareResult <= 0))
                {
                    continue;
                }
            }

            if (pRelatedMsi->fMaxProvided)
            {
                hr = VerCompareParsedVersions(pVersion, pRelatedMsi->pMaxVersion, &nCompareResult);
                ExitOnFailure(hr, "Failed to compare related installed version '%ls' to related max version: '%ls'", pVersion->sczVersion, pRelatedMsi->pMaxVersion->sczVersion);

                if (pRelatedMsi->fMaxInclusive ? (nCompareResult > 0) : (nCompareResult >= 0))
                {
                    continue;
                }
            }

            // Filter by language if necessary.
            uLcid = 0; // always reset the found language.
            if (pRelatedMsi->cLanguages)
            {
                // If there is a language to get, convert it into an LCID.
                hr = WiuGetProductInfoEx(wzProductCode, NULL, fPerMachine ? MSIINSTALLCONTEXT_MACHINE : MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_LANGUAGE, &sczInstalledLanguage);
                if (SUCCEEDED(hr))
                {
                    hr = StrStringToUInt32(sczInstalledLanguage, 0, &uLcid);
                }

                // Ignore related product where we can't read the language.
                if (FAILED(hr))
                {
                    LogErrorId(hr, MSG_FAILED_READ_RELATED_PACKAGE_LANGUAGE, wzProductCode, sczInstalledLanguage, NULL);

                    hr = S_OK;
                    continue;
                }

                BOOL fMatchedLcid = FALSE;
                for (DWORD iLanguage = 0; iLanguage < pRelatedMsi->cLanguages; ++iLanguage)
                {
                    if (uLcid == pRelatedMsi->rgdwLanguages[iLanguage])
                    {
                        fMatchedLcid = TRUE;
                        break;
                    }
                }

                // Skip the product if the language did not meet the inclusive/exclusive criteria.
                if ((pRelatedMsi->fLangInclusive && !fMatchedLcid) || (!pRelatedMsi->fLangInclusive && fMatchedLcid))
                {
                    continue;
                }
            }

            // If this is a detect-only related package and we're not installed yet, then we'll assume a downgrade
            // would take place since that is the overwhelmingly common use of detect-only related packages. If
            // not detect-only then it's easy; we're clearly doing a major upgrade.
            if (pRelatedMsi->fOnlyDetect)
            {
                // If we've already detected a major upgrade that trumps any guesses that the detect is a downgrade
                // or even something else.
                if (BOOTSTRAPPER_RELATED_OPERATION_MAJOR_UPGRADE == operation)
                {
                    relatedMsiOperation = BOOTSTRAPPER_RELATED_OPERATION_NONE;
                }
                // It can't be a downgrade if the upgrade codes aren't the same.
                else if (BOOTSTRAPPER_PACKAGE_STATE_ABSENT == pPackage->currentState &&
                         pPackage->Msi.sczUpgradeCode && CSTR_EQUAL == ::CompareStringW(LOCALE_NEUTRAL, NORM_IGNORECASE, pPackage->Msi.sczUpgradeCode, -1, pRelatedMsi->sczUpgradeCode, -1))
                {
                    relatedMsiOperation = BOOTSTRAPPER_RELATED_OPERATION_DOWNGRADE;
                    operation = BOOTSTRAPPER_RELATED_OPERATION_DOWNGRADE;
                    pPackage->currentState = BOOTSTRAPPER_PACKAGE_STATE_OBSOLETE;
                }
                else // we're already on the machine so the detect-only *must* be for detection purposes only.
                {
                    relatedMsiOperation = BOOTSTRAPPER_RELATED_OPERATION_NONE;
                }
            }
            else
            {
                relatedMsiOperation = BOOTSTRAPPER_RELATED_OPERATION_MAJOR_UPGRADE;
                operation = BOOTSTRAPPER_RELATED_OPERATION_MAJOR_UPGRADE;
            }

            LogId(REPORT_STANDARD, MSG_DETECTED_RELATED_PACKAGE, wzProductCode, LoggingPerMachineToString(fPerMachine), pVersion->sczVersion, uLcid, LoggingRelatedOperationToString(relatedMsiOperation));

            // Pass to BA.
            hr = UserExperienceOnDetectRelatedMsiPackage(pUserExperience, pPackage->sczId, pRelatedMsi->sczUpgradeCode, wzProductCode, fPerMachine, pVersion, relatedMsiOperation);
            ExitOnRootFailure(hr, "BA aborted detect related MSI package.");
        }
    }

    // detect features
    if (pPackage->Msi.cFeatures)
    {
        for (DWORD i = 0; i < pPackage->Msi.cFeatures; ++i)
        {
            BURN_MSIFEATURE* pFeature = &pPackage->Msi.rgFeatures[i];

            // Try to detect features state if the product is present on the machine.
            if (BOOTSTRAPPER_PACKAGE_STATE_PRESENT <= pPackage->currentState)
            {
                hr = WiuQueryFeatureState(pPackage->Msi.sczProductCode, pFeature->sczId, &installState);
                ExitOnFailure(hr, "Failed to query feature state.");

                if (INSTALLSTATE_UNKNOWN == installState) // in case of an upgrade a feature could be removed.
                {
                    installState = INSTALLSTATE_ABSENT;
                }
            }
            else // MSI not installed then the features can't be either.
            {
                installState = INSTALLSTATE_ABSENT;
            }

            // set current state
            switch (installState)
            {
            case INSTALLSTATE_ABSENT:
                pFeature->currentState = BOOTSTRAPPER_FEATURE_STATE_ABSENT;
                break;
            case INSTALLSTATE_ADVERTISED:
                pFeature->currentState = BOOTSTRAPPER_FEATURE_STATE_ADVERTISED;
                break;
            case INSTALLSTATE_LOCAL:
                pFeature->currentState = BOOTSTRAPPER_FEATURE_STATE_LOCAL;
                break;
            case INSTALLSTATE_SOURCE:
                pFeature->currentState = BOOTSTRAPPER_FEATURE_STATE_SOURCE;
                break;
            default:
                hr = E_UNEXPECTED;
                ExitOnRootFailure(hr, "Invalid state value.");
            }

            // Pass to BA.
            hr = UserExperienceOnDetectMsiFeature(pUserExperience, pPackage->sczId, pFeature->sczId, pFeature->currentState);
            ExitOnRootFailure(hr, "BA aborted detect MSI feature.");
        }
    }

    if (pPackage->fCanAffectRegistration)
    {
        pPackage->installRegistrationState = BOOTSTRAPPER_PACKAGE_STATE_CACHED < pPackage->currentState ? BURN_PACKAGE_REGISTRATION_STATE_PRESENT : BURN_PACKAGE_REGISTRATION_STATE_ABSENT;
    }

LExit:
    ReleaseStr(sczInstalledLanguage);
    ReleaseStr(sczInstalledVersion);
    ReleaseVerutilVersion(pVersion);

    return hr;
}

extern "C" HRESULT MsiEnginePlanInitializePackage(
    __in BURN_PACKAGE* pPackage,
    __in BURN_VARIABLES* pVariables,
    __in BURN_USER_EXPERIENCE* pUserExperience
    )
{
    HRESULT hr = S_OK;

    if (pPackage->Msi.cFeatures)
    {
        // get feature request states
        for (DWORD i = 0; i < pPackage->Msi.cFeatures; ++i)
        {
            BURN_MSIFEATURE* pFeature = &pPackage->Msi.rgFeatures[i];

            // Evaluate feature conditions.
            hr = EvaluateActionStateConditions(pVariables, pFeature->sczAddLocalCondition, pFeature->sczAddSourceCondition, pFeature->sczAdvertiseCondition, &pFeature->defaultRequested);
            ExitOnFailure(hr, "Failed to evaluate requested state conditions.");

            hr = EvaluateActionStateConditions(pVariables, pFeature->sczRollbackAddLocalCondition, pFeature->sczRollbackAddSourceCondition, pFeature->sczRollbackAdvertiseCondition, &pFeature->expectedState);
            ExitOnFailure(hr, "Failed to evaluate expected state conditions.");

            // Remember the default feature requested state so the engine doesn't get blamed for planning the wrong thing if the BA changes it.
            pFeature->requested = pFeature->defaultRequested;

            // Send plan MSI feature message to BA.
            hr = UserExperienceOnPlanMsiFeature(pUserExperience, pPackage->sczId, pFeature->sczId, &pFeature->requested);
            ExitOnRootFailure(hr, "BA aborted plan MSI feature.");
        }
    }

LExit:
    return hr;
}

//
// PlanCalculate - calculates the execute and rollback state for the requested package state.
//
extern "C" HRESULT MsiEnginePlanCalculatePackage(
    __in BURN_PACKAGE* pPackage,
    __in BOOL fInsideMsiTransaction
    )
{
    Trace(REPORT_STANDARD, "Planning MSI package 0x%p", pPackage);

    HRESULT hr = S_OK;
    VERUTIL_VERSION* pVersion = pPackage->Msi.pVersion;
    VERUTIL_VERSION* pInstalledVersion = pPackage->Msi.pInstalledVersion;
    int nCompareResult = 0;
    BOOTSTRAPPER_ACTION_STATE execute = BOOTSTRAPPER_ACTION_STATE_NONE;
    BOOTSTRAPPER_ACTION_STATE rollback = BOOTSTRAPPER_ACTION_STATE_NONE;
    BOOL fFeatureActionDelta = FALSE;
    BOOL fRollbackFeatureActionDelta = FALSE;

    if (pPackage->Msi.cFeatures)
    {
        // If the package is present and we're repairing it.
        BOOL fRepairingPackage = (BOOTSTRAPPER_PACKAGE_STATE_CACHED < pPackage->currentState && BOOTSTRAPPER_REQUEST_STATE_REPAIR == pPackage->requested);

        // plan features
        for (DWORD i = 0; i < pPackage->Msi.cFeatures; ++i)
        {
            BURN_MSIFEATURE* pFeature = &pPackage->Msi.rgFeatures[i];

            // Calculate feature actions.
            hr = CalculateFeatureAction(pFeature->currentState, pFeature->requested, fRepairingPackage, &pFeature->execute, &fFeatureActionDelta);
            ExitOnFailure(hr, "Failed to calculate execute feature state.");

            hr = CalculateFeatureAction(pFeature->requested, BOOTSTRAPPER_FEATURE_ACTION_NONE == pFeature->execute ? pFeature->expectedState : pFeature->currentState, FALSE, &pFeature->rollback, &fRollbackFeatureActionDelta);
            ExitOnFailure(hr, "Failed to calculate rollback feature state.");
        }
    }

    // execute action
    switch (pPackage->currentState)
    {
    case BOOTSTRAPPER_PACKAGE_STATE_PRESENT: __fallthrough;
    case BOOTSTRAPPER_PACKAGE_STATE_SUPERSEDED:
        if (BOOTSTRAPPER_REQUEST_STATE_PRESENT == pPackage->requested || BOOTSTRAPPER_REQUEST_STATE_MEND == pPackage->requested || BOOTSTRAPPER_REQUEST_STATE_REPAIR == pPackage->requested)
        {
            hr = VerCompareParsedVersions(pVersion, pInstalledVersion, &nCompareResult);
            ExitOnFailure(hr, "Failed to compare '%ls' to '%ls' for planning.", pVersion->sczVersion, pInstalledVersion->sczVersion);

            // Take a look at the version and determine if this is a potential
            // minor upgrade (same ProductCode newer ProductVersion), otherwise,
            // there is a newer version so no work necessary.
            if (nCompareResult > 0)
            {
                execute = BOOTSTRAPPER_ACTION_STATE_MINOR_UPGRADE;
            }
            else if (BOOTSTRAPPER_REQUEST_STATE_MEND == pPackage->requested)
            {
                execute = BOOTSTRAPPER_ACTION_STATE_MEND;
            }
            else if (BOOTSTRAPPER_REQUEST_STATE_REPAIR == pPackage->requested)
            {
                execute = BOOTSTRAPPER_ACTION_STATE_REPAIR;
            }
            else
            {
                execute = fFeatureActionDelta ? BOOTSTRAPPER_ACTION_STATE_MODIFY : BOOTSTRAPPER_ACTION_STATE_NONE;
            }
        }
        else if ((BOOTSTRAPPER_REQUEST_STATE_ABSENT == pPackage->requested || BOOTSTRAPPER_REQUEST_STATE_CACHE == pPackage->requested) &&
                 pPackage->fUninstallable) // removing a package that can be removed.
        {
            execute = BOOTSTRAPPER_ACTION_STATE_UNINSTALL;
        }
        else if (BOOTSTRAPPER_REQUEST_STATE_FORCE_ABSENT == pPackage->requested)
        {
            execute = BOOTSTRAPPER_ACTION_STATE_UNINSTALL;
        }
        else
        {
            execute = BOOTSTRAPPER_ACTION_STATE_NONE;
        }
        break;

    case BOOTSTRAPPER_PACKAGE_STATE_CACHED:
        switch (pPackage->requested)
        {
        case BOOTSTRAPPER_REQUEST_STATE_PRESENT: __fallthrough;
        case BOOTSTRAPPER_REQUEST_STATE_MEND: __fallthrough;
        case BOOTSTRAPPER_REQUEST_STATE_REPAIR:
            execute = BOOTSTRAPPER_ACTION_STATE_INSTALL;
            break;

        default:
            execute = BOOTSTRAPPER_ACTION_STATE_NONE;
            break;
        }
        break;

    case BOOTSTRAPPER_PACKAGE_STATE_OBSOLETE: __fallthrough;
    case BOOTSTRAPPER_PACKAGE_STATE_ABSENT:
        switch (pPackage->requested)
        {
        case BOOTSTRAPPER_REQUEST_STATE_PRESENT: __fallthrough;
        case BOOTSTRAPPER_REQUEST_STATE_MEND: __fallthrough;
        case BOOTSTRAPPER_REQUEST_STATE_REPAIR:
            execute = BOOTSTRAPPER_ACTION_STATE_INSTALL;
            break;

        default:
            execute = BOOTSTRAPPER_ACTION_STATE_NONE;
            break;
        }
        break;

    default:
        hr = E_INVALIDARG;
        ExitOnRootFailure(hr, "Invalid package current state result encountered during plan: %d", pPackage->currentState);
    }

    // Calculate the rollback action if there is an execute action.
    if (BOOTSTRAPPER_ACTION_STATE_NONE != execute && !fInsideMsiTransaction)
    {
        switch (pPackage->currentState)
        {
        case BOOTSTRAPPER_PACKAGE_STATE_PRESENT: __fallthrough;
        case BOOTSTRAPPER_PACKAGE_STATE_SUPERSEDED:
            switch (pPackage->requested)
            {
            case BOOTSTRAPPER_REQUEST_STATE_PRESENT:
                rollback = fRollbackFeatureActionDelta ? BOOTSTRAPPER_ACTION_STATE_MODIFY : BOOTSTRAPPER_ACTION_STATE_NONE;
                break;
            case BOOTSTRAPPER_REQUEST_STATE_REPAIR:
                rollback = BOOTSTRAPPER_ACTION_STATE_NONE;
                break;
            case BOOTSTRAPPER_REQUEST_STATE_FORCE_ABSENT: __fallthrough;
            case BOOTSTRAPPER_REQUEST_STATE_ABSENT:
                rollback = BOOTSTRAPPER_ACTION_STATE_INSTALL;
                break;
            default:
                rollback = BOOTSTRAPPER_ACTION_STATE_NONE;
                break;
            }
            break;

        case BOOTSTRAPPER_PACKAGE_STATE_OBSOLETE: __fallthrough;
        case BOOTSTRAPPER_PACKAGE_STATE_ABSENT: __fallthrough;
        case BOOTSTRAPPER_PACKAGE_STATE_CACHED:
            // If the package is uninstallable and we requested to put the package on the machine then
            // remove the package during rollback.
            if (pPackage->fUninstallable &&
                (BOOTSTRAPPER_REQUEST_STATE_PRESENT == pPackage->requested ||
                 BOOTSTRAPPER_REQUEST_STATE_MEND == pPackage->requested ||
                 BOOTSTRAPPER_REQUEST_STATE_REPAIR == pPackage->requested))
            {
                rollback = BOOTSTRAPPER_ACTION_STATE_UNINSTALL;
            }
            else
            {
                rollback = BOOTSTRAPPER_ACTION_STATE_NONE;
            }
            break;

        default:
            hr = E_INVALIDARG;
            ExitOnRootFailure(hr, "Invalid package detection result encountered.");
        }
    }

    // return values
    pPackage->execute = execute;
    pPackage->rollback = rollback;

LExit:
    return hr;
}

//
// PlanAdd - adds the calculated execute and rollback actions for the package.
//
extern "C" HRESULT MsiEnginePlanAddPackage(
    __in BOOTSTRAPPER_DISPLAY display,
    __in BURN_USER_EXPERIENCE* pUserExperience,
    __in BURN_PACKAGE* pPackage,
    __in BURN_PLAN* pPlan,
    __in BURN_LOGGING* pLog,
    __in BURN_VARIABLES* pVariables,
    __in_opt HANDLE hCacheEvent
    )
{
    HRESULT hr = S_OK;
    BURN_EXECUTE_ACTION* pAction = NULL;
    BOOTSTRAPPER_FEATURE_ACTION* rgFeatureActions = NULL;
    BOOTSTRAPPER_FEATURE_ACTION* rgRollbackFeatureActions = NULL;

    if (pPackage->Msi.cFeatures)
    {
        // Allocate and populate array for feature actions.
        rgFeatureActions = (BOOTSTRAPPER_FEATURE_ACTION*)MemAlloc(sizeof(BOOTSTRAPPER_FEATURE_ACTION) * pPackage->Msi.cFeatures, TRUE);
        ExitOnNull(rgFeatureActions, hr, E_OUTOFMEMORY, "Failed to allocate memory for feature actions.");

        rgRollbackFeatureActions = (BOOTSTRAPPER_FEATURE_ACTION*)MemAlloc(sizeof(BOOTSTRAPPER_FEATURE_ACTION) * pPackage->Msi.cFeatures, TRUE);
        ExitOnNull(rgRollbackFeatureActions, hr, E_OUTOFMEMORY, "Failed to allocate memory for rollback feature actions.");

        for (DWORD i = 0; i < pPackage->Msi.cFeatures; ++i)
        {
            BURN_MSIFEATURE* pFeature = &pPackage->Msi.rgFeatures[i];

            // calculate feature actions
            rgFeatureActions[i] = pFeature->execute;
            rgRollbackFeatureActions[i] = pFeature->rollback;
        }
    }

    // add wait for cache
    if (hCacheEvent)
    {
        hr = PlanExecuteCacheSyncAndRollback(pPlan, pPackage, hCacheEvent);
        ExitOnFailure(hr, "Failed to plan package cache syncpoint");
    }

    hr = DependencyPlanPackage(NULL, pPackage, pPlan);
    ExitOnFailure(hr, "Failed to plan package dependency actions.");

    // add rollback action
    if (BOOTSTRAPPER_ACTION_STATE_NONE != pPackage->rollback)
    {
        hr = PlanAppendRollbackAction(pPlan, &pAction);
        ExitOnFailure(hr, "Failed to append rollback action.");

        pAction->type = BURN_EXECUTE_ACTION_TYPE_MSI_PACKAGE;
        pAction->msiPackage.pPackage = pPackage;
        pAction->msiPackage.action = pPackage->rollback;
        pAction->msiPackage.rgFeatures = rgRollbackFeatureActions;
        rgRollbackFeatureActions = NULL;

        hr = MsiEngineCalculateInstallUiLevel(display, pUserExperience, pPackage->sczId, FALSE, pAction->msiPackage.action,
            &pAction->msiPackage.actionMsiProperty, &pAction->msiPackage.uiLevel, &pAction->msiPackage.fDisableExternalUiHandler);
        ExitOnFailure(hr, "Failed to get msi ui options.");

        LoggingSetPackageVariable(pPackage, NULL, TRUE, pLog, pVariables, &pAction->msiPackage.sczLogPath); // ignore errors.
        pAction->msiPackage.dwLoggingAttributes = pLog->dwAttributes;

        // Plan a checkpoint between rollback and execute so that we always attempt
        // rollback in the case that the MSI was not able to rollback itself (e.g.
        // user pushes cancel after InstallFinalize).
        hr = PlanExecuteCheckpoint(pPlan);
        ExitOnFailure(hr, "Failed to append execute checkpoint.");
    }

    // add execute action
    if (BOOTSTRAPPER_ACTION_STATE_NONE != pPackage->execute)
    {
        hr = PlanAppendExecuteAction(pPlan, &pAction);
        ExitOnFailure(hr, "Failed to append execute action.");

        pAction->type = BURN_EXECUTE_ACTION_TYPE_MSI_PACKAGE;
        pAction->msiPackage.pPackage = pPackage;
        pAction->msiPackage.action = pPackage->execute;
        pAction->msiPackage.rgFeatures = rgFeatureActions;
        rgFeatureActions = NULL;

        hr = MsiEngineCalculateInstallUiLevel(display, pUserExperience, pPackage->sczId, TRUE, pAction->msiPackage.action,
            &pAction->msiPackage.actionMsiProperty, &pAction->msiPackage.uiLevel, &pAction->msiPackage.fDisableExternalUiHandler);
        ExitOnFailure(hr, "Failed to get msi ui options.");

        LoggingSetPackageVariable(pPackage, NULL, FALSE, pLog, pVariables, &pAction->msiPackage.sczLogPath); // ignore errors.
        pAction->msiPackage.dwLoggingAttributes = pLog->dwAttributes;
    }

LExit:
    ReleaseMem(rgFeatureActions);
    ReleaseMem(rgRollbackFeatureActions);

    return hr;
}

extern "C" HRESULT MsiEngineBeginTransaction(
    __in LPCWSTR wzName,
    __out MSIHANDLE *phTransactionHandle,
    __out HANDLE *phChangeOfOwnerEvent,
    __in_z LPCWSTR szLogPath
    )
{
    HRESULT hr = S_OK;

    hr = WiuBeginTransaction(wzName, 0, phTransactionHandle, phChangeOfOwnerEvent, WIU_LOG_DEFAULT | INSTALLLOGMODE_VERBOSE, szLogPath);

    if (HRESULT_FROM_WIN32(ERROR_ROLLBACK_DISABLED) == hr)
    {
        LogId(REPORT_ERROR, MSG_MSI_TRANSACTIONS_DISABLED);
    }

    ExitOnFailure(hr, "Failed to begin an MSI transaction");

LExit:
    return hr;
}

extern "C" HRESULT MsiEngineCommitTransaction(
    __inout MSIHANDLE * phTransactionHandle,
    __inout HANDLE * phChangeOfOwnerEvent,
    __in_z LPCWSTR szLogPath
    )
{
    HRESULT hr = S_OK;

    hr = WiuEndTransaction(MSITRANSACTIONSTATE_COMMIT, WIU_LOG_DEFAULT | INSTALLLOGMODE_VERBOSE, szLogPath);
    ExitOnFailure(hr, "Failed to commit the MSI transaction");

    if (phChangeOfOwnerEvent && *phChangeOfOwnerEvent && INVALID_HANDLE_VALUE != *phChangeOfOwnerEvent)
    {
        ::CloseHandle(phChangeOfOwnerEvent);
        *phChangeOfOwnerEvent = NULL;
    }
    if (phTransactionHandle && *phTransactionHandle)
    {
        ::MsiCloseHandle(*phTransactionHandle);
        *phTransactionHandle = NULL;
    }

LExit:
    return hr;
}

extern "C" HRESULT MsiEngineRollbackTransaction(
    __inout MSIHANDLE *phTransactionHandle,
    __inout HANDLE *phChangeOfOwnerEvent,
    __in_z LPCWSTR szLogPath
    )
{
    HRESULT hr = S_OK;

    hr = WiuEndTransaction(MSITRANSACTIONSTATE_ROLLBACK, WIU_LOG_DEFAULT | INSTALLLOGMODE_VERBOSE, szLogPath);
    ExitOnFailure(hr, "Failed to rollback the MSI transaction");

    if (phChangeOfOwnerEvent && *phChangeOfOwnerEvent && INVALID_HANDLE_VALUE != *phChangeOfOwnerEvent)
    {
        ::CloseHandle(phChangeOfOwnerEvent);
        *phChangeOfOwnerEvent = NULL;
    }
    if (phTransactionHandle && *phTransactionHandle)
    {
        ::MsiCloseHandle(*phTransactionHandle);
        *phTransactionHandle = NULL;
    }

LExit:

    return hr;
}

extern "C" HRESULT MsiEngineExecutePackage(
    __in_opt HWND hwndParent,
    __in BURN_EXECUTE_ACTION* pExecuteAction,
    __in BURN_VARIABLES* pVariables,
    __in BOOL fRollback,
    __in PFN_MSIEXECUTEMESSAGEHANDLER pfnMessageHandler,
    __in LPVOID pvContext,
    __out BOOTSTRAPPER_APPLY_RESTART* pRestart
    )
{
    HRESULT hr = S_OK;
    WIU_MSI_EXECUTE_CONTEXT context = { };
    WIU_RESTART restart = WIU_RESTART_NONE;

    LPWSTR sczInstalledVersion = NULL;
    LPWSTR sczCachedDirectory = NULL;
    LPWSTR sczMsiPath = NULL;
    LPWSTR sczProperties = NULL;
    LPWSTR sczObfuscatedProperties = NULL;
    BURN_PACKAGE* pPackage = pExecuteAction->msiPackage.pPackage;
    BURN_PAYLOAD* pPackagePayload = pPackage->payloads.rgItems[0].pPayload;

    // During rollback, if the package is already in the rollback state we expect don't
    // touch it again.
    if (fRollback)
    {
        if (BOOTSTRAPPER_ACTION_STATE_UNINSTALL == pExecuteAction->msiPackage.action)
        {
            hr = WiuGetProductInfoEx(pPackage->Msi.sczProductCode, NULL, pPackage->fPerMachine ? MSIINSTALLCONTEXT_MACHINE : MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_VERSIONSTRING, &sczInstalledVersion);
            if (FAILED(hr))  // package not present.
            {
                LogId(REPORT_STANDARD, MSG_ROLLBACK_PACKAGE_SKIPPED, pPackage->sczId, LoggingActionStateToString(pExecuteAction->msiPackage.action), LoggingPackageStateToString(BOOTSTRAPPER_PACKAGE_STATE_ABSENT));

                hr = S_OK;
                ExitFunction();
            }
        }
        else if (BOOTSTRAPPER_ACTION_STATE_INSTALL == pExecuteAction->msiPackage.action)
        {
            hr = WiuGetProductInfoEx(pPackage->Msi.sczProductCode, NULL, pPackage->fPerMachine ? MSIINSTALLCONTEXT_MACHINE : MSIINSTALLCONTEXT_USERUNMANAGED, INSTALLPROPERTY_VERSIONSTRING, &sczInstalledVersion);
            if (SUCCEEDED(hr))  // package present.
            {
                LogId(REPORT_STANDARD, MSG_ROLLBACK_PACKAGE_SKIPPED, pPackage->sczId, LoggingActionStateToString(pExecuteAction->msiPackage.action), LoggingPackageStateToString(BOOTSTRAPPER_PACKAGE_STATE_PRESENT));

                hr = S_OK;
                ExitFunction();
            }

            hr = S_OK;
        }
    }

    // Default to "verbose" logging and set extra debug mode only if explicitly required.
    DWORD dwLogMode = WIU_LOG_DEFAULT | INSTALLLOGMODE_VERBOSE;

    if (pExecuteAction->msiPackage.dwLoggingAttributes & BURN_LOGGING_ATTRIBUTE_EXTRADEBUG)
    {
        dwLogMode |= INSTALLLOGMODE_EXTRADEBUG;
    }

    if (BOOTSTRAPPER_ACTION_STATE_UNINSTALL != pExecuteAction->msiPackage.action)
    {
        // get cached MSI path
        hr = CacheGetCompletedPath(pPackage->fPerMachine, pPackage->sczCacheId, &sczCachedDirectory);
        ExitOnFailure(hr, "Failed to get cached path for package: %ls", pPackage->sczId);

        // Best effort to set the execute package cache folder variable.
        VariableSetString(pVariables, BURN_BUNDLE_EXECUTE_PACKAGE_CACHE_FOLDER, sczCachedDirectory, TRUE, FALSE);

        hr = PathConcat(sczCachedDirectory, pPackagePayload->sczFilePath, &sczMsiPath);
        ExitOnFailure(hr, "Failed to build MSI path.");
    }

    // Best effort to set the execute package action variable.
    VariableSetNumeric(pVariables, BURN_BUNDLE_EXECUTE_PACKAGE_ACTION, pExecuteAction->msiPackage.action, TRUE);
    
    // Wire up the external UI handler and logging.
    if (pExecuteAction->msiPackage.fDisableExternalUiHandler)
    {
        hr = WiuInitializeInternalUI(pExecuteAction->msiPackage.uiLevel, hwndParent, &context);
        ExitOnFailure(hr, "Failed to initialize internal UI for MSI package.");
    }
    else
    {
        hr = WiuInitializeExternalUI(pfnMessageHandler, pExecuteAction->msiPackage.uiLevel, hwndParent, pvContext, fRollback, &context);
        ExitOnFailure(hr, "Failed to initialize external UI handler.");
    }

    if (pExecuteAction->msiPackage.sczLogPath && *pExecuteAction->msiPackage.sczLogPath)
    {
        hr = WiuEnableLog(dwLogMode, pExecuteAction->msiPackage.sczLogPath, 0);
        ExitOnFailure(hr, "Failed to enable logging for package: %ls to: %ls", pPackage->sczId, pExecuteAction->msiPackage.sczLogPath);
    }

    // set up properties
    hr = MsiEngineConcatProperties(pPackage->Msi.rgProperties, pPackage->Msi.cProperties, pVariables, fRollback, &sczProperties, FALSE);
    ExitOnFailure(hr, "Failed to add properties to argument string.");

    hr = MsiEngineConcatProperties(pPackage->Msi.rgProperties, pPackage->Msi.cProperties, pVariables, fRollback, &sczObfuscatedProperties, TRUE);
    ExitOnFailure(hr, "Failed to add obfuscated properties to argument string.");

    // add feature action properties
    hr = ConcatFeatureActionProperties(pPackage, pExecuteAction->msiPackage.rgFeatures, &sczProperties);
    ExitOnFailure(hr, "Failed to add feature action properties to argument string.");

    hr = ConcatFeatureActionProperties(pPackage, pExecuteAction->msiPackage.rgFeatures, &sczObfuscatedProperties);
    ExitOnFailure(hr, "Failed to add feature action properties to obfuscated argument string.");

    // add slipstream patch properties
    hr = ConcatPatchProperty(pPackage, fRollback, &sczProperties);
    ExitOnFailure(hr, "Failed to add patch properties to argument string.");

    hr = ConcatPatchProperty(pPackage, fRollback, &sczObfuscatedProperties);
    ExitOnFailure(hr, "Failed to add patch properties to obfuscated argument string.");

    hr = MsiEngineConcatActionProperty(pExecuteAction->msiPackage.actionMsiProperty, &sczProperties);
    ExitOnFailure(hr, "Failed to add action property to argument string.");

    hr = MsiEngineConcatActionProperty(pExecuteAction->msiPackage.actionMsiProperty, &sczObfuscatedProperties);
    ExitOnFailure(hr, "Failed to add action property to obfuscated argument string.");

    LogId(REPORT_STANDARD, MSG_APPLYING_PACKAGE, LoggingRollbackOrExecute(fRollback), pPackage->sczId, LoggingActionStateToString(pExecuteAction->msiPackage.action), sczMsiPath, sczObfuscatedProperties ? sczObfuscatedProperties : L"");

    //
    // Do the actual action.
    //
    switch (pExecuteAction->msiPackage.action)
    {
    case BOOTSTRAPPER_ACTION_STATE_INSTALL:
        hr = StrAllocConcatSecure(&sczProperties, L" REBOOT=ReallySuppress", 0);
        ExitOnFailure(hr, "Failed to add reboot suppression property on install.");

        hr = WiuInstallProduct(sczMsiPath, sczProperties, &restart);
        ExitOnFailure(hr, "Failed to install MSI package.");

        RegisterSourceDirectory(pPackage, sczMsiPath);
        break;

    case BOOTSTRAPPER_ACTION_STATE_MINOR_UPGRADE:
        // If feature selection is not enabled, then reinstall the existing features to ensure they get
        // updated.
        if (0 == pPackage->Msi.cFeatures)
        {
            hr = StrAllocConcatSecure(&sczProperties, L" REINSTALL=ALL", 0);
            ExitOnFailure(hr, "Failed to add reinstall all property on minor upgrade.");
        }

        hr = StrAllocConcatSecure(&sczProperties, L" REINSTALLMODE=\"vomus\" REBOOT=ReallySuppress", 0);
        ExitOnFailure(hr, "Failed to add reinstall mode and reboot suppression properties on minor upgrade.");

        hr = WiuInstallProduct(sczMsiPath, sczProperties, &restart);
        ExitOnFailure(hr, "Failed to perform minor upgrade of MSI package.");

        RegisterSourceDirectory(pPackage, sczMsiPath);
        break;

    case BOOTSTRAPPER_ACTION_STATE_MODIFY: __fallthrough;
    case BOOTSTRAPPER_ACTION_STATE_MEND: __fallthrough;
    case BOOTSTRAPPER_ACTION_STATE_REPAIR:
        {
        LPCWSTR wzReinstallAll = (BOOTSTRAPPER_ACTION_STATE_MODIFY == pExecuteAction->msiPackage.action ||
                                  pPackage->Msi.cFeatures) ? L"" : L" REINSTALL=ALL";
        LPCWSTR wzReinstallMode = (BOOTSTRAPPER_ACTION_STATE_MODIFY == pExecuteAction->msiPackage.action || BOOTSTRAPPER_ACTION_STATE_MEND == pExecuteAction->msiPackage.action) ? L"o" : L"e";

        hr = StrAllocFormattedSecure(&sczProperties, L"%ls%ls REINSTALLMODE=\"cmus%ls\" REBOOT=ReallySuppress", sczProperties ? sczProperties : L"", wzReinstallAll, wzReinstallMode);
        ExitOnFailure(hr, "Failed to add reinstall mode and reboot suppression properties on repair.");
        }

        // Ignore all dependencies, since the Burn engine already performed the check.
        hr = StrAllocFormattedSecure(&sczProperties, L"%ls %ls=ALL", sczProperties, DEPENDENCY_IGNOREDEPENDENCIES);
        ExitOnFailure(hr, "Failed to add the list of dependencies to ignore to the properties.");

        hr = WiuInstallProduct(sczMsiPath, sczProperties, &restart);
        ExitOnFailure(hr, "Failed to run maintenance mode for MSI package.");
        break;

    case BOOTSTRAPPER_ACTION_STATE_UNINSTALL:
        hr = StrAllocConcatSecure(&sczProperties, L" REBOOT=ReallySuppress", 0);
        ExitOnFailure(hr, "Failed to add reboot suppression property on uninstall.");

        // Ignore all dependencies, since the Burn engine already performed the check.
        hr = StrAllocFormattedSecure(&sczProperties, L"%ls %ls=ALL", sczProperties, DEPENDENCY_IGNOREDEPENDENCIES);
        ExitOnFailure(hr, "Failed to add the list of dependencies to ignore to the properties.");

        hr = WiuConfigureProductEx(pPackage->Msi.sczProductCode, INSTALLLEVEL_DEFAULT, INSTALLSTATE_ABSENT, sczProperties, &restart);
        if (HRESULT_FROM_WIN32(ERROR_UNKNOWN_PRODUCT) == hr)
        {
            LogId(REPORT_STANDARD, MSG_ATTEMPTED_UNINSTALL_ABSENT_PACKAGE, pPackage->sczId);
            hr = S_OK;
        }
        ExitOnFailure(hr, "Failed to uninstall MSI package.");
        break;
    }

LExit:
    WiuUninitializeExternalUI(&context);

    StrSecureZeroFreeString(sczProperties);
    ReleaseStr(sczObfuscatedProperties);
    ReleaseStr(sczMsiPath);
    ReleaseStr(sczCachedDirectory);
    ReleaseStr(sczInstalledVersion);

    switch (restart)
    {
        case WIU_RESTART_NONE:
            *pRestart = BOOTSTRAPPER_APPLY_RESTART_NONE;
            break;

        case WIU_RESTART_REQUIRED:
            *pRestart = BOOTSTRAPPER_APPLY_RESTART_REQUIRED;
            break;

        case WIU_RESTART_INITIATED:
            *pRestart = BOOTSTRAPPER_APPLY_RESTART_INITIATED;
            break;
    }

    // Best effort to clear the execute package cache folder and action variables.
    VariableSetString(pVariables, BURN_BUNDLE_EXECUTE_PACKAGE_CACHE_FOLDER, NULL, TRUE, FALSE);
    VariableSetString(pVariables, BURN_BUNDLE_EXECUTE_PACKAGE_ACTION, NULL, TRUE, FALSE);

    return hr;
}

extern "C" HRESULT MsiEngineConcatActionProperty(
    __in BURN_MSI_PROPERTY actionMsiProperty,
    __deref_out_z LPWSTR* psczProperties
    )
{
    HRESULT hr = S_OK;
    LPCWSTR wzPropertyName = NULL;

    switch (actionMsiProperty)
    {
    case BURN_MSI_PROPERTY_INSTALL:
        wzPropertyName = BURNMSIINSTALL_PROPERTY_NAME;
        break;
    case BURN_MSI_PROPERTY_MODIFY:
        wzPropertyName = BURNMSIMODIFY_PROPERTY_NAME;
        break;
    case BURN_MSI_PROPERTY_REPAIR:
        wzPropertyName = BURNMSIREPAIR_PROPERTY_NAME;
        break;
    case BURN_MSI_PROPERTY_UNINSTALL:
        wzPropertyName = BURNMSIUNINSTALL_PROPERTY_NAME;
        break;
    }

    if (wzPropertyName)
    {
        hr = StrAllocConcatFormattedSecure(psczProperties, L" %ls=1", wzPropertyName);
        ExitOnFailure(hr, "Failed to add burn action property.");
    }

LExit:
    return hr;
}

extern "C" HRESULT MsiEngineConcatProperties(
    __in_ecount(cProperties) BURN_MSIPROPERTY* rgProperties,
    __in DWORD cProperties,
    __in BURN_VARIABLES* pVariables,
    __in BOOL fRollback,
    __deref_out_z LPWSTR* psczProperties,
    __in BOOL fObfuscateHiddenVariables
    )
{
    HRESULT hr = S_OK;
    LPWSTR sczValue = NULL;
    LPWSTR sczEscapedValue = NULL;
    LPWSTR sczProperty = NULL;

    for (DWORD i = 0; i < cProperties; ++i)
    {
        BURN_MSIPROPERTY* pProperty = &rgProperties[i];

        if (pProperty->sczCondition && *pProperty->sczCondition)
        {
            BOOL fCondition = FALSE;

            hr = ConditionEvaluate(pVariables, pProperty->sczCondition, &fCondition);
            if (FAILED(hr) || !fCondition)
            {
                LogId(REPORT_VERBOSE, MSG_MSI_PROPERTY_CONDITION_FAILED, pProperty->sczId, pProperty->sczCondition, LoggingTrueFalseToString(fCondition));
                continue;
            }
        }

        // format property value
        if (fObfuscateHiddenVariables)
        {
            hr = VariableFormatStringObfuscated(pVariables, (fRollback && pProperty->sczRollbackValue) ? pProperty->sczRollbackValue : pProperty->sczValue, &sczValue, NULL);
        }
        else
        {
            hr = VariableFormatString(pVariables, (fRollback && pProperty->sczRollbackValue) ? pProperty->sczRollbackValue : pProperty->sczValue, &sczValue, NULL);
            ExitOnFailure(hr, "Failed to format property value.");
        }
        ExitOnFailure(hr, "Failed to format property value.");

        // escape property value
        hr = EscapePropertyArgumentString(sczValue, &sczEscapedValue, !fObfuscateHiddenVariables);
        ExitOnFailure(hr, "Failed to escape string.");

        // build part
        hr = VariableStrAllocFormatted(!fObfuscateHiddenVariables, &sczProperty, L" %s%=\"%s\"", pProperty->sczId, sczEscapedValue);
        ExitOnFailure(hr, "Failed to format property string part.");

        // append to property string
        hr = VariableStrAllocConcat(!fObfuscateHiddenVariables, psczProperties, sczProperty, 0);
        ExitOnFailure(hr, "Failed to append property string part.");
    }

LExit:
    StrSecureZeroFreeString(sczValue);
    StrSecureZeroFreeString(sczEscapedValue);
    StrSecureZeroFreeString(sczProperty);
    return hr;
}

extern "C" HRESULT MsiEngineCalculateInstallUiLevel(
    __in BOOTSTRAPPER_DISPLAY display,
    __in BURN_USER_EXPERIENCE* pUserExperience,
    __in LPCWSTR wzPackageId,
    __in BOOL fExecute,
    __in BOOTSTRAPPER_ACTION_STATE actionState,
    __out BURN_MSI_PROPERTY* pActionMsiProperty,
    __out INSTALLUILEVEL* pUiLevel,
    __out BOOL* pfDisableExternalUiHandler
    )
{
    *pUiLevel = INSTALLUILEVEL_NONE;
    *pfDisableExternalUiHandler = FALSE;

    if (BOOTSTRAPPER_DISPLAY_FULL == display ||
        BOOTSTRAPPER_DISPLAY_PASSIVE == display)
    {
        *pUiLevel = static_cast<INSTALLUILEVEL>(*pUiLevel | INSTALLUILEVEL_SOURCERESONLY);
    }

    switch (actionState)
    {
    case BOOTSTRAPPER_ACTION_STATE_UNINSTALL:
        *pActionMsiProperty = BURN_MSI_PROPERTY_UNINSTALL;
        break;
    case BOOTSTRAPPER_ACTION_STATE_REPAIR:
        *pActionMsiProperty = BURN_MSI_PROPERTY_REPAIR;
        break;
    case BOOTSTRAPPER_ACTION_STATE_MODIFY:
        *pActionMsiProperty = BURN_MSI_PROPERTY_MODIFY;
        break;
    default:
        *pActionMsiProperty = BURN_MSI_PROPERTY_INSTALL;
        break;
    }

    return UserExperienceOnPlanMsiPackage(pUserExperience, wzPackageId, fExecute, actionState, pActionMsiProperty, pUiLevel, pfDisableExternalUiHandler);
}

extern "C" void MsiEngineUpdateInstallRegistrationState(
    __in BURN_EXECUTE_ACTION* pAction,
    __in BOOL fRollback,
    __in HRESULT hrExecute,
    __in BOOL fInsideMsiTransaction
    )
{
    BURN_PACKAGE_REGISTRATION_STATE newState = BURN_PACKAGE_REGISTRATION_STATE_UNKNOWN;
    BURN_PACKAGE* pPackage = pAction->msiPackage.pPackage;

    if (FAILED(hrExecute) || !pPackage->fCanAffectRegistration)
    {
        ExitFunction();
    }

    if (BOOTSTRAPPER_ACTION_STATE_UNINSTALL == pAction->msiPackage.action)
    {
        newState = BURN_PACKAGE_REGISTRATION_STATE_ABSENT;
    }
    else
    {
        newState = BURN_PACKAGE_REGISTRATION_STATE_PRESENT;
    }

    if (fInsideMsiTransaction)
    {
        pPackage->transactionRegistrationState = newState;
    }
    else
    {
        pPackage->installRegistrationState = newState;
    }

    if (BURN_PACKAGE_REGISTRATION_STATE_ABSENT == newState)
    {
        for (DWORD i = 0; i < pPackage->Msi.cChainedPatches; ++i)
        {
            BURN_CHAINED_PATCH* pChainedPatch = pPackage->Msi.rgChainedPatches + i;
            BURN_MSPTARGETPRODUCT* pTargetProduct = pChainedPatch->pMspPackage->Msp.rgTargetProducts + pChainedPatch->dwMspTargetProductIndex;

            if (fInsideMsiTransaction)
            {
                pTargetProduct->transactionRegistrationState = newState;
            }
            else
            {
                pTargetProduct->registrationState = newState;
            }
        }
    }
    else
    {
        for (DWORD i = 0; i < pPackage->Msi.cSlipstreamMspPackages; ++i)
        {
            BURN_SLIPSTREAM_MSP* pSlipstreamMsp = pPackage->Msi.rgSlipstreamMsps + i;
            BOOTSTRAPPER_ACTION_STATE patchExecuteAction = fRollback ? pSlipstreamMsp->rollback : pSlipstreamMsp->execute;

            if (BOOTSTRAPPER_ACTION_STATE_INSTALL > patchExecuteAction)
            {
                continue;
            }

            BURN_CHAINED_PATCH* pChainedPatch = pPackage->Msi.rgChainedPatches + pSlipstreamMsp->dwMsiChainedPatchIndex;
            BURN_MSPTARGETPRODUCT* pTargetProduct = pChainedPatch->pMspPackage->Msp.rgTargetProducts + pChainedPatch->dwMspTargetProductIndex;

            if (fInsideMsiTransaction)
            {
                pTargetProduct->transactionRegistrationState = newState;
            }
            else
            {
                pTargetProduct->registrationState = newState;
            }
        }
    }

LExit:
    return;
}


// internal helper functions

static HRESULT ParseRelatedMsiFromXml(
    __in IXMLDOMNode* pixnRelatedMsi,
    __in BURN_RELATED_MSI* pRelatedMsi
    )
{
    HRESULT hr = S_OK;
    IXMLDOMNodeList* pixnNodes = NULL;
    IXMLDOMNode* pixnNode = NULL;
    DWORD cNodes = 0;
    LPWSTR scz = NULL;

    // @Id
    hr = XmlGetAttributeEx(pixnRelatedMsi, L"Id", &pRelatedMsi->sczUpgradeCode);
    ExitOnFailure(hr, "Failed to get @Id.");

    // @MinVersion
    hr = XmlGetAttributeEx(pixnRelatedMsi, L"MinVersion", &scz);
    if (E_NOTFOUND != hr)
    {
        ExitOnFailure(hr, "Failed to get @MinVersion.");

        hr = VerParseVersion(scz, 0, FALSE, &pRelatedMsi->pMinVersion);
        ExitOnFailure(hr, "Failed to parse @MinVersion: %ls", scz);

        if (pRelatedMsi->pMinVersion->fInvalid)
        {
            LogId(REPORT_WARNING, MSG_MANIFEST_INVALID_VERSION, scz);
        }

        // flag that we have a min version
        pRelatedMsi->fMinProvided = TRUE;

        // @MinInclusive
        hr = XmlGetYesNoAttribute(pixnRelatedMsi, L"MinInclusive", &pRelatedMsi->fMinInclusive);
        ExitOnFailure(hr, "Failed to get @MinInclusive.");
    }

    // @MaxVersion
    hr = XmlGetAttributeEx(pixnRelatedMsi, L"MaxVersion", &scz);
    if (E_NOTFOUND != hr)
    {
        ExitOnFailure(hr, "Failed to get @MaxVersion.");

        hr = VerParseVersion(scz, 0, FALSE, &pRelatedMsi->pMaxVersion);
        ExitOnFailure(hr, "Failed to parse @MaxVersion: %ls", scz);

        if (pRelatedMsi->pMaxVersion->fInvalid)
        {
            LogId(REPORT_WARNING, MSG_MANIFEST_INVALID_VERSION, scz);
        }

        // flag that we have a max version
        pRelatedMsi->fMaxProvided = TRUE;

        // @MaxInclusive
        hr = XmlGetYesNoAttribute(pixnRelatedMsi, L"MaxInclusive", &pRelatedMsi->fMaxInclusive);
        ExitOnFailure(hr, "Failed to get @MaxInclusive.");
    }

    // @OnlyDetect
    hr = XmlGetYesNoAttribute(pixnRelatedMsi, L"OnlyDetect", &pRelatedMsi->fOnlyDetect);
    ExitOnFailure(hr, "Failed to get @OnlyDetect.");

    // select language nodes
    hr = XmlSelectNodes(pixnRelatedMsi, L"Language", &pixnNodes);
    ExitOnFailure(hr, "Failed to select language nodes.");

    // get language node count
    hr = pixnNodes->get_length((long*)&cNodes);
    ExitOnFailure(hr, "Failed to get language node count.");

    if (cNodes)
    {
        // @LangInclusive
        hr = XmlGetYesNoAttribute(pixnRelatedMsi, L"LangInclusive", &pRelatedMsi->fLangInclusive);
        ExitOnFailure(hr, "Failed to get @LangInclusive.");

        // allocate memory for language IDs
        pRelatedMsi->rgdwLanguages = (DWORD*)MemAlloc(sizeof(DWORD) * cNodes, TRUE);
        ExitOnNull(pRelatedMsi->rgdwLanguages, hr, E_OUTOFMEMORY, "Failed to allocate memory for language IDs.");

        pRelatedMsi->cLanguages = cNodes;

        // parse language elements
        for (DWORD i = 0; i < cNodes; ++i)
        {
            hr = XmlNextElement(pixnNodes, &pixnNode, NULL);
            ExitOnFailure(hr, "Failed to get next node.");

            // @Id
            hr = XmlGetAttributeNumber(pixnNode, L"Id", &pRelatedMsi->rgdwLanguages[i]);
            ExitOnFailure(hr, "Failed to get Language/@Id.");

            // prepare next iteration
            ReleaseNullObject(pixnNode);
        }
    }

    hr = S_OK;

LExit:
    ReleaseObject(pixnNodes);
    ReleaseObject(pixnNode);
    ReleaseStr(scz);

    return hr;
}

static HRESULT EvaluateActionStateConditions(
    __in BURN_VARIABLES* pVariables,
    __in_z_opt LPCWSTR sczAddLocalCondition,
    __in_z_opt LPCWSTR sczAddSourceCondition,
    __in_z_opt LPCWSTR sczAdvertiseCondition,
    __out BOOTSTRAPPER_FEATURE_STATE* pState
    )
{
    HRESULT hr = S_OK;
    BOOL fCondition = FALSE;

    // if no condition was set, return no feature state
    if (!sczAddLocalCondition && !sczAddSourceCondition && !sczAdvertiseCondition)
    {
        *pState = BOOTSTRAPPER_FEATURE_STATE_UNKNOWN;
        ExitFunction();
    }

    if (sczAddLocalCondition)
    {
        hr = ConditionEvaluate(pVariables, sczAddLocalCondition, &fCondition);
        ExitOnFailure(hr, "Failed to evaluate add local condition.");

        if (fCondition)
        {
            *pState = BOOTSTRAPPER_FEATURE_STATE_LOCAL;
            ExitFunction();
        }
    }

    if (sczAddSourceCondition)
    {
        hr = ConditionEvaluate(pVariables, sczAddSourceCondition, &fCondition);
        ExitOnFailure(hr, "Failed to evaluate add source condition.");

        if (fCondition)
        {
            *pState = BOOTSTRAPPER_FEATURE_STATE_SOURCE;
            ExitFunction();
        }
    }

    if (sczAdvertiseCondition)
    {
        hr = ConditionEvaluate(pVariables, sczAdvertiseCondition, &fCondition);
        ExitOnFailure(hr, "Failed to evaluate advertise condition.");

        if (fCondition)
        {
            *pState = BOOTSTRAPPER_FEATURE_STATE_ADVERTISED;
            ExitFunction();
        }
    }

    // if no condition was true, set to absent
    *pState = BOOTSTRAPPER_FEATURE_STATE_ABSENT;

LExit:
    return hr;
}

static HRESULT CalculateFeatureAction(
    __in BOOTSTRAPPER_FEATURE_STATE currentState,
    __in BOOTSTRAPPER_FEATURE_STATE requestedState,
    __in BOOL fRepair,
    __out BOOTSTRAPPER_FEATURE_ACTION* pFeatureAction,
    __inout BOOL* pfDelta
    )
{
    HRESULT hr = S_OK;

    *pFeatureAction = BOOTSTRAPPER_FEATURE_ACTION_NONE;
    switch (requestedState)
    {
    case BOOTSTRAPPER_FEATURE_STATE_UNKNOWN:
        *pFeatureAction = BOOTSTRAPPER_FEATURE_ACTION_NONE;
        break;

    case BOOTSTRAPPER_FEATURE_STATE_ABSENT:
        if (BOOTSTRAPPER_FEATURE_STATE_ABSENT != currentState)
        {
            *pFeatureAction = BOOTSTRAPPER_FEATURE_ACTION_REMOVE;
        }
        break;

    case BOOTSTRAPPER_FEATURE_STATE_ADVERTISED:
        if (BOOTSTRAPPER_FEATURE_STATE_ADVERTISED != currentState)
        {
            *pFeatureAction = BOOTSTRAPPER_FEATURE_ACTION_ADVERTISE;
        }
        else if (fRepair)
        {
            *pFeatureAction = BOOTSTRAPPER_FEATURE_ACTION_REINSTALL;
        }
        break;

    case BOOTSTRAPPER_FEATURE_STATE_LOCAL:
        if (BOOTSTRAPPER_FEATURE_STATE_LOCAL != currentState)
        {
            *pFeatureAction = BOOTSTRAPPER_FEATURE_ACTION_ADDLOCAL;
        }
        else if (fRepair)
        {
            *pFeatureAction = BOOTSTRAPPER_FEATURE_ACTION_REINSTALL;
        }
        break;

    case BOOTSTRAPPER_FEATURE_STATE_SOURCE:
        if (BOOTSTRAPPER_FEATURE_STATE_SOURCE != currentState)
        {
            *pFeatureAction = BOOTSTRAPPER_FEATURE_ACTION_ADDSOURCE;
        }
        else if (fRepair)
        {
            *pFeatureAction = BOOTSTRAPPER_FEATURE_ACTION_REINSTALL;
        }
        break;

    default:
        hr = E_UNEXPECTED;
        ExitOnRootFailure(hr, "Invalid state value.");
    }

    if (BOOTSTRAPPER_FEATURE_ACTION_NONE != *pFeatureAction)
    {
        *pfDelta = TRUE;
    }

LExit:
    return hr;
}

static HRESULT EscapePropertyArgumentString(
    __in LPCWSTR wzProperty,
    __inout_z LPWSTR* psczEscapedValue,
    __in BOOL fZeroOnRealloc
    )
{
    HRESULT hr = S_OK;
    DWORD cch = 0;
    DWORD cchEscape = 0;
    LPCWSTR wzSource = NULL;
    LPWSTR wzTarget = NULL;

    // count characters to escape
    wzSource = wzProperty;
    while (*wzSource)
    {
        ++cch;
        if (L'\"' == *wzSource)
        {
            ++cchEscape;
        }
        ++wzSource;
    }

    // allocate target buffer
    hr = VariableStrAlloc(fZeroOnRealloc, psczEscapedValue, cch + cchEscape + 1); // character count, plus escape character count, plus null terminator
    ExitOnFailure(hr, "Failed to allocate string buffer.");

    // write to target buffer
    wzSource = wzProperty;
    wzTarget = *psczEscapedValue;
    while (*wzSource)
    {
        *wzTarget = *wzSource;
        if (L'\"' == *wzTarget)
        {
            ++wzTarget;
            *wzTarget = L'\"';
        }

        ++wzSource;
        ++wzTarget;
    }

    *wzTarget = L'\0'; // add null terminator

LExit:
    return hr;
}

static HRESULT ConcatFeatureActionProperties(
    __in BURN_PACKAGE* pPackage,
    __in BOOTSTRAPPER_FEATURE_ACTION* rgFeatureActions,
    __inout_z LPWSTR* psczArguments
    )
{
    HRESULT hr = S_OK;
    LPWSTR scz = NULL;
    LPWSTR sczAddLocal = NULL;
    LPWSTR sczAddSource = NULL;
    LPWSTR sczAddDefault = NULL;
    LPWSTR sczReinstall = NULL;
    LPWSTR sczAdvertise = NULL;
    LPWSTR sczRemove = NULL;

    // features
    for (DWORD i = 0; i < pPackage->Msi.cFeatures; ++i)
    {
        BURN_MSIFEATURE* pFeature = &pPackage->Msi.rgFeatures[i];

        switch (rgFeatureActions[i])
        {
        case BOOTSTRAPPER_FEATURE_ACTION_ADDLOCAL:
            if (sczAddLocal)
            {
                hr = StrAllocConcat(&sczAddLocal, L",", 0);
                ExitOnFailure(hr, "Failed to concat separator.");
            }
            hr = StrAllocConcat(&sczAddLocal, pFeature->sczId, 0);
            ExitOnFailure(hr, "Failed to concat feature.");
            break;

        case BOOTSTRAPPER_FEATURE_ACTION_ADDSOURCE:
            if (sczAddSource)
            {
                hr = StrAllocConcat(&sczAddSource, L",", 0);
                ExitOnFailure(hr, "Failed to concat separator.");
            }
            hr = StrAllocConcat(&sczAddSource, pFeature->sczId, 0);
            ExitOnFailure(hr, "Failed to concat feature.");
            break;

        case BOOTSTRAPPER_FEATURE_ACTION_ADDDEFAULT:
            if (sczAddDefault)
            {
                hr = StrAllocConcat(&sczAddDefault, L",", 0);
                ExitOnFailure(hr, "Failed to concat separator.");
            }
            hr = StrAllocConcat(&sczAddDefault, pFeature->sczId, 0);
            ExitOnFailure(hr, "Failed to concat feature.");
            break;

        case BOOTSTRAPPER_FEATURE_ACTION_REINSTALL:
            if (sczReinstall)
            {
                hr = StrAllocConcat(&sczReinstall, L",", 0);
                ExitOnFailure(hr, "Failed to concat separator.");
            }
            hr = StrAllocConcat(&sczReinstall, pFeature->sczId, 0);
            ExitOnFailure(hr, "Failed to concat feature.");
            break;

        case BOOTSTRAPPER_FEATURE_ACTION_ADVERTISE:
            if (sczAdvertise)
            {
                hr = StrAllocConcat(&sczAdvertise, L",", 0);
                ExitOnFailure(hr, "Failed to concat separator.");
            }
            hr = StrAllocConcat(&sczAdvertise, pFeature->sczId, 0);
            ExitOnFailure(hr, "Failed to concat feature.");
            break;

        case BOOTSTRAPPER_FEATURE_ACTION_REMOVE:
            if (sczRemove)
            {
                hr = StrAllocConcat(&sczRemove, L",", 0);
                ExitOnFailure(hr, "Failed to concat separator.");
            }
            hr = StrAllocConcat(&sczRemove, pFeature->sczId, 0);
            ExitOnFailure(hr, "Failed to concat feature.");
            break;
        }
    }

    if (sczAddLocal)
    {
        hr = StrAllocFormatted(&scz, L" ADDLOCAL=\"%s\"", sczAddLocal, 0);
        ExitOnFailure(hr, "Failed to format ADDLOCAL string.");

        hr = StrAllocConcatSecure(psczArguments, scz, 0);
        ExitOnFailure(hr, "Failed to concat argument string.");
    }

    if (sczAddSource)
    {
        hr = StrAllocFormatted(&scz, L" ADDSOURCE=\"%s\"", sczAddSource, 0);
        ExitOnFailure(hr, "Failed to format ADDSOURCE string.");

        hr = StrAllocConcatSecure(psczArguments, scz, 0);
        ExitOnFailure(hr, "Failed to concat argument string.");
    }

    if (sczAddDefault)
    {
        hr = StrAllocFormatted(&scz, L" ADDDEFAULT=\"%s\"", sczAddDefault, 0);
        ExitOnFailure(hr, "Failed to format ADDDEFAULT string.");

        hr = StrAllocConcatSecure(psczArguments, scz, 0);
        ExitOnFailure(hr, "Failed to concat argument string.");
    }

    if (sczReinstall)
    {
        hr = StrAllocFormatted(&scz, L" REINSTALL=\"%s\"", sczReinstall, 0);
        ExitOnFailure(hr, "Failed to format REINSTALL string.");

        hr = StrAllocConcatSecure(psczArguments, scz, 0);
        ExitOnFailure(hr, "Failed to concat argument string.");
    }

    if (sczAdvertise)
    {
        hr = StrAllocFormatted(&scz, L" ADVERTISE=\"%s\"", sczAdvertise, 0);
        ExitOnFailure(hr, "Failed to format ADVERTISE string.");

        hr = StrAllocConcatSecure(psczArguments, scz, 0);
        ExitOnFailure(hr, "Failed to concat argument string.");
    }

    if (sczRemove)
    {
        hr = StrAllocFormatted(&scz, L" REMOVE=\"%s\"", sczRemove, 0);
        ExitOnFailure(hr, "Failed to format REMOVE string.");

        hr = StrAllocConcatSecure(psczArguments, scz, 0);
        ExitOnFailure(hr, "Failed to concat argument string.");
    }

LExit:
    ReleaseStr(scz);
    ReleaseStr(sczAddLocal);
    ReleaseStr(sczAddSource);
    ReleaseStr(sczAddDefault);
    ReleaseStr(sczReinstall);
    ReleaseStr(sczAdvertise);
    ReleaseStr(sczRemove);

    return hr;
}

static HRESULT ConcatPatchProperty(
    __in BURN_PACKAGE* pPackage,
    __in BOOL fRollback,
    __inout_z LPWSTR* psczArguments
    )
{
    HRESULT hr = S_OK;
    LPWSTR sczCachedDirectory = NULL;
    LPWSTR sczMspPath = NULL;
    LPWSTR sczPatches = NULL;

    // If there are slipstream patch actions, build up their patch action.
    if (pPackage->Msi.cSlipstreamMspPackages)
    {
        for (DWORD i = 0; i < pPackage->Msi.cSlipstreamMspPackages; ++i)
        {
            BURN_SLIPSTREAM_MSP* pSlipstreamMsp = pPackage->Msi.rgSlipstreamMsps + i;
            BURN_PACKAGE* pMspPackage = pSlipstreamMsp->pMspPackage;
            BURN_PAYLOAD* pMspPackagePayload = pMspPackage->payloads.rgItems[0].pPayload;
            BOOTSTRAPPER_ACTION_STATE patchExecuteAction = fRollback ? pSlipstreamMsp->rollback : pSlipstreamMsp->execute;

            if (BOOTSTRAPPER_ACTION_STATE_UNINSTALL < patchExecuteAction)
            {
                hr = CacheGetCompletedPath(pMspPackage->fPerMachine, pMspPackage->sczCacheId, &sczCachedDirectory);
                ExitOnFailure(hr, "Failed to get cached path for MSP package: %ls", pMspPackage->sczId);

                hr = PathConcat(sczCachedDirectory, pMspPackagePayload->sczFilePath, &sczMspPath);
                ExitOnFailure(hr, "Failed to build MSP path.");

                if (!sczPatches)
                {
                    hr = StrAllocConcat(&sczPatches, L" PATCH=\"", 0);
                    ExitOnFailure(hr, "Failed to prefix with PATCH property.");
                }
                else
                {
                    hr = StrAllocConcat(&sczPatches, L";", 0);
                    ExitOnFailure(hr, "Failed to semi-colon delimit patches.");
                }

                hr = StrAllocConcat(&sczPatches, sczMspPath, 0);
                ExitOnFailure(hr, "Failed to append patch path.");
            }
        }

        if (sczPatches)
        {
            hr = StrAllocConcat(&sczPatches, L"\"", 0);
            ExitOnFailure(hr, "Failed to close the quoted PATCH property.");

            hr = StrAllocConcatSecure(psczArguments, sczPatches, 0);
            ExitOnFailure(hr, "Failed to append PATCH property.");
        }
    }

LExit:
    ReleaseStr(sczMspPath);
    ReleaseStr(sczCachedDirectory);
    ReleaseStr(sczPatches);
    return hr;
}

static void RegisterSourceDirectory(
    __in BURN_PACKAGE* pPackage,
    __in_z LPCWSTR wzMsiPath
    )
{
    HRESULT hr = S_OK;
    LPWSTR sczMsiDirectory = NULL;
    MSIINSTALLCONTEXT dwContext = pPackage->fPerMachine ? MSIINSTALLCONTEXT_MACHINE : MSIINSTALLCONTEXT_USERUNMANAGED;

    hr = PathGetDirectory(wzMsiPath, &sczMsiDirectory);
    ExitOnFailure(hr, "Failed to get directory for path: %ls", wzMsiPath);

    hr = WiuSourceListAddSourceEx(pPackage->Msi.sczProductCode, NULL, dwContext, MSICODE_PRODUCT, sczMsiDirectory, 1);
    if (FAILED(hr))
    {
        LogId(REPORT_VERBOSE, MSG_SOURCELIST_REGISTER, sczMsiDirectory, pPackage->Msi.sczProductCode, hr);
        ExitFunction();
    }

LExit:
    ReleaseStr(sczMsiDirectory);

    return;
}
