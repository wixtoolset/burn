// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.

#include "precomp.h"

static HRESULT WINAPI PlanTestBAProc(
    __in BOOTSTRAPPER_APPLICATION_MESSAGE message,
    __in const LPVOID pvArgs,
    __inout LPVOID pvResults,
    __in_opt LPVOID pvContext
    );

static LPCWSTR wzMsiTransactionManifestFileName = L"MsiTransaction_BundleAv1_manifest.xml";
static LPCWSTR wzSingleMsiManifestFileName = L"BasicFunctionality_BundleA_manifest.xml";
static LPCWSTR wzSlipstreamManifestFileName = L"Slipstream_BundleA_manifest.xml";

namespace Microsoft
{
namespace Tools
{
namespace WindowsInstallerXml
{
namespace Test
{
namespace Bootstrapper
{
    using namespace System;
    using namespace Xunit;

    public ref class PlanTest : BurnUnitTest
    {
    public:
        PlanTest(BurnTestFixture^ fixture) : BurnUnitTest(fixture)
        {
        }

        [Fact]
        void MsiTransactionInstallTest()
        {
            HRESULT hr = S_OK;
            BURN_ENGINE_STATE engineState = { };
            BURN_ENGINE_STATE* pEngineState = &engineState;
            BURN_PLAN* pPlan = &engineState.plan;

            InitializeEngineStateForCorePlan(wzMsiTransactionManifestFileName, pEngineState);
            DetectPackagesAsAbsent(pEngineState);
            DetectUpgradeBundle(pEngineState, L"{FD9920AD-DBCA-4C6C-8CD5-B47431CE8D21}", L"1.0.0.0");

            hr = CorePlan(pEngineState, BOOTSTRAPPER_ACTION_INSTALL);
            NativeAssert::Succeeded(hr, "CorePlan failed");

            Assert::Equal<DWORD>(BOOTSTRAPPER_ACTION_INSTALL, pPlan->action);
            Assert::Equal<BOOL>(TRUE, pPlan->fPerMachine);
            Assert::Equal<BOOL>(FALSE, pPlan->fDisableRollback);

            BOOL fRollback = FALSE;
            DWORD dwIndex = 0;
            ValidateCacheCheckpoint(pPlan, fRollback, dwIndex++, 1);
            ValidateCachePackage(pPlan, fRollback, dwIndex++, L"PackageA");
            ValidateCacheSignalSyncpoint(pPlan, fRollback, dwIndex++);
            ValidateCacheCheckpoint(pPlan, fRollback, dwIndex++, 9);
            ValidateCachePackage(pPlan, fRollback, dwIndex++, L"PackageB");
            ValidateCacheSignalSyncpoint(pPlan, fRollback, dwIndex++);
            ValidateCacheCheckpoint(pPlan, fRollback, dwIndex++, 14);
            ValidateCachePackage(pPlan, fRollback, dwIndex++, L"PackageC");
            ValidateCacheSignalSyncpoint(pPlan, fRollback, dwIndex++);
            Assert::Equal(dwIndex, pPlan->cCacheActions);

            fRollback = TRUE;
            dwIndex = 0;
            ValidateCacheCheckpoint(pPlan, fRollback, dwIndex++, 1);
            Assert::Equal(dwIndex, pPlan->cRollbackCacheActions);

            Assert::Equal(107082ull, pPlan->qwEstimatedSize);
            Assert::Equal(506145ull, pPlan->qwCacheSizeTotal);

            fRollback = FALSE;
            dwIndex = 0;
            DWORD dwExecuteCheckpointId = 2;
            ValidateExecuteRollbackBoundary(pPlan, fRollback, dwIndex++, L"WixDefaultBoundary", TRUE, FALSE);
            ValidateExecuteWaitSyncpoint(pPlan, fRollback, dwIndex++, pPlan->rgCacheActions[2].syncpoint.hEvent);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageProvider(pPlan, fRollback, dwIndex++, L"PackageA", BURN_DEPENDENCY_ACTION_REGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteMsiPackage(pPlan, fRollback, dwIndex++, L"PackageA", BOOTSTRAPPER_ACTION_STATE_INSTALL, BURN_MSI_PROPERTY_INSTALL, INSTALLUILEVEL_NONE, FALSE, 0);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PackageA", L"{E6469F05-BDC8-4EB8-B218-67412543EFAA}", BURN_DEPENDENCY_ACTION_REGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteRollbackBoundary(pPlan, fRollback, dwIndex++, L"rbaOCA08D8ky7uBOK71_6FWz1K3TuQ", TRUE, TRUE);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteBeginMsiTransaction(pPlan, fRollback, dwIndex++, L"rbaOCA08D8ky7uBOK71_6FWz1K3TuQ");
            ValidateExecuteWaitSyncpoint(pPlan, fRollback, dwIndex++, pPlan->rgCacheActions[5].syncpoint.hEvent);
            dwExecuteCheckpointId += 1; // cache checkpoints
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageProvider(pPlan, fRollback, dwIndex++, L"PackageB", BURN_DEPENDENCY_ACTION_REGISTER);
            ValidateExecuteMsiPackage(pPlan, fRollback, dwIndex++, L"PackageB", BOOTSTRAPPER_ACTION_STATE_INSTALL, BURN_MSI_PROPERTY_INSTALL, INSTALLUILEVEL_NONE, FALSE, 0);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PackageB", L"{E6469F05-BDC8-4EB8-B218-67412543EFAA}", BURN_DEPENDENCY_ACTION_REGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteWaitSyncpoint(pPlan, fRollback, dwIndex++, pPlan->rgCacheActions[8].syncpoint.hEvent);
            dwExecuteCheckpointId += 1; // cache checkpoints
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageProvider(pPlan, fRollback, dwIndex++, L"PackageC", BURN_DEPENDENCY_ACTION_REGISTER);
            ValidateExecuteMsiPackage(pPlan, fRollback, dwIndex++, L"PackageC", BOOTSTRAPPER_ACTION_STATE_INSTALL, BURN_MSI_PROPERTY_INSTALL, INSTALLUILEVEL_NONE, FALSE, 0);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PackageC", L"{E6469F05-BDC8-4EB8-B218-67412543EFAA}", BURN_DEPENDENCY_ACTION_REGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCommitMsiTransaction(pPlan, fRollback, dwIndex++, L"rbaOCA08D8ky7uBOK71_6FWz1K3TuQ");
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteExePackage(pPlan, fRollback, dwIndex++, L"{FD9920AD-DBCA-4C6C-8CD5-B47431CE8D21}", BOOTSTRAPPER_ACTION_STATE_UNINSTALL, NULL);
            Assert::Equal(dwIndex, pPlan->cExecuteActions);

            fRollback = TRUE;
            dwIndex = 0;
            dwExecuteCheckpointId = 2;
            ValidateExecuteRollbackBoundary(pPlan, fRollback, dwIndex++, L"WixDefaultBoundary", TRUE, FALSE);
            ValidateExecuteUncachePackage(pPlan, fRollback, dwIndex++, L"PackageA");
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageProvider(pPlan, fRollback, dwIndex++, L"PackageA", BURN_DEPENDENCY_ACTION_UNREGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteMsiPackage(pPlan, fRollback, dwIndex++, L"PackageA", BOOTSTRAPPER_ACTION_STATE_UNINSTALL, BURN_MSI_PROPERTY_UNINSTALL, INSTALLUILEVEL_NONE, FALSE, 0);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PackageA", L"{E6469F05-BDC8-4EB8-B218-67412543EFAA}", BURN_DEPENDENCY_ACTION_UNREGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteRollbackBoundary(pPlan, fRollback, dwIndex++, L"rbaOCA08D8ky7uBOK71_6FWz1K3TuQ", TRUE, TRUE);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteUncachePackage(pPlan, fRollback, dwIndex++, L"PackageB");
            dwExecuteCheckpointId += 1; // cache checkpoints
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageProvider(pPlan, fRollback, dwIndex++, L"PackageB", BURN_DEPENDENCY_ACTION_UNREGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PackageB", L"{E6469F05-BDC8-4EB8-B218-67412543EFAA}", BURN_DEPENDENCY_ACTION_UNREGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteUncachePackage(pPlan, fRollback, dwIndex++, L"PackageC");
            dwExecuteCheckpointId += 1; // cache checkpoints
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageProvider(pPlan, fRollback, dwIndex++, L"PackageC", BURN_DEPENDENCY_ACTION_UNREGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PackageC", L"{E6469F05-BDC8-4EB8-B218-67412543EFAA}", BURN_DEPENDENCY_ACTION_UNREGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteExePackage(pPlan, fRollback, dwIndex++, L"{FD9920AD-DBCA-4C6C-8CD5-B47431CE8D21}", BOOTSTRAPPER_ACTION_STATE_INSTALL, NULL);
            Assert::Equal(dwIndex, pPlan->cRollbackActions);

            Assert::Equal(4ul, pPlan->cExecutePackagesTotal);
            Assert::Equal(7ul, pPlan->cOverallProgressTicksTotal);

            dwIndex = 0;
            Assert::Equal(dwIndex, pPlan->cCleanActions);

            UINT uIndex = 0;
            ValidatePlannedProvider(pPlan, uIndex++, L"{E6469F05-BDC8-4EB8-B218-67412543EFAA}", NULL);
            Assert::Equal(uIndex, pPlan->cPlannedProviders);

            Assert::Equal(3ul, pEngineState->packages.cPackages);
            ValidateNonPermanentPackageExpectedStates(&pEngineState->packages.rgPackages[0], L"PackageA", BURN_PACKAGE_REGISTRATION_STATE_PRESENT, BURN_PACKAGE_REGISTRATION_STATE_PRESENT);
            ValidateNonPermanentPackageExpectedStates(&pEngineState->packages.rgPackages[1], L"PackageB", BURN_PACKAGE_REGISTRATION_STATE_PRESENT, BURN_PACKAGE_REGISTRATION_STATE_PRESENT);
            ValidateNonPermanentPackageExpectedStates(&pEngineState->packages.rgPackages[2], L"PackageC", BURN_PACKAGE_REGISTRATION_STATE_PRESENT, BURN_PACKAGE_REGISTRATION_STATE_PRESENT);
        }

        [Fact]
        void MsiTransactionUninstallTest()
        {
            HRESULT hr = S_OK;
            BURN_ENGINE_STATE engineState = { };
            BURN_ENGINE_STATE* pEngineState = &engineState;
            BURN_PLAN* pPlan = &engineState.plan;

            InitializeEngineStateForCorePlan(wzMsiTransactionManifestFileName, pEngineState);
            DetectPackagesAsPresentAndCached(pEngineState);

            hr = CorePlan(pEngineState, BOOTSTRAPPER_ACTION_UNINSTALL);
            NativeAssert::Succeeded(hr, "CorePlan failed");

            Assert::Equal<DWORD>(BOOTSTRAPPER_ACTION_UNINSTALL, pPlan->action);
            Assert::Equal<BOOL>(TRUE, pPlan->fPerMachine);
            Assert::Equal<BOOL>(FALSE, pPlan->fDisableRollback);

            BOOL fRollback = FALSE;
            DWORD dwIndex = 0;
            Assert::Equal(dwIndex, pPlan->cCacheActions);

            fRollback = TRUE;
            dwIndex = 0;
            Assert::Equal(dwIndex, pPlan->cRollbackCacheActions);

            Assert::Equal(0ull, pPlan->qwEstimatedSize);
            Assert::Equal(0ull, pPlan->qwCacheSizeTotal);

            fRollback = FALSE;
            dwIndex = 0;
            DWORD dwExecuteCheckpointId = 1;
            ValidateExecuteRollbackBoundary(pPlan, fRollback, dwIndex++, L"rbaOCA08D8ky7uBOK71_6FWz1K3TuQ", TRUE, TRUE);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteBeginMsiTransaction(pPlan, fRollback, dwIndex++, L"rbaOCA08D8ky7uBOK71_6FWz1K3TuQ");
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PackageC", L"{E6469F05-BDC8-4EB8-B218-67412543EFAA}", BURN_DEPENDENCY_ACTION_UNREGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageProvider(pPlan, fRollback, dwIndex++, L"PackageC", BURN_DEPENDENCY_ACTION_UNREGISTER);
            ValidateExecuteMsiPackage(pPlan, fRollback, dwIndex++, L"PackageC", BOOTSTRAPPER_ACTION_STATE_UNINSTALL, BURN_MSI_PROPERTY_UNINSTALL, INSTALLUILEVEL_NONE, FALSE, 0);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PackageB", L"{E6469F05-BDC8-4EB8-B218-67412543EFAA}", BURN_DEPENDENCY_ACTION_UNREGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageProvider(pPlan, fRollback, dwIndex++, L"PackageB", BURN_DEPENDENCY_ACTION_UNREGISTER);
            ValidateExecuteMsiPackage(pPlan, fRollback, dwIndex++, L"PackageB", BOOTSTRAPPER_ACTION_STATE_UNINSTALL, BURN_MSI_PROPERTY_UNINSTALL, INSTALLUILEVEL_NONE, FALSE, 0);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCommitMsiTransaction(pPlan, fRollback, dwIndex++, L"rbaOCA08D8ky7uBOK71_6FWz1K3TuQ");
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteRollbackBoundary(pPlan, fRollback, dwIndex++, L"WixDefaultBoundary", TRUE, FALSE);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PackageA", L"{E6469F05-BDC8-4EB8-B218-67412543EFAA}", BURN_DEPENDENCY_ACTION_UNREGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageProvider(pPlan, fRollback, dwIndex++, L"PackageA", BURN_DEPENDENCY_ACTION_UNREGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteMsiPackage(pPlan, fRollback, dwIndex++, L"PackageA", BOOTSTRAPPER_ACTION_STATE_UNINSTALL, BURN_MSI_PROPERTY_UNINSTALL, INSTALLUILEVEL_NONE, FALSE, 0);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            Assert::Equal(dwIndex, pPlan->cExecuteActions);

            fRollback = TRUE;
            dwIndex = 0;
            dwExecuteCheckpointId = 1;
            ValidateExecuteRollbackBoundary(pPlan, fRollback, dwIndex++, L"rbaOCA08D8ky7uBOK71_6FWz1K3TuQ", TRUE, TRUE);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PackageC", L"{E6469F05-BDC8-4EB8-B218-67412543EFAA}", BURN_DEPENDENCY_ACTION_REGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageProvider(pPlan, fRollback, dwIndex++, L"PackageC", BURN_DEPENDENCY_ACTION_REGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PackageB", L"{E6469F05-BDC8-4EB8-B218-67412543EFAA}", BURN_DEPENDENCY_ACTION_REGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageProvider(pPlan, fRollback, dwIndex++, L"PackageB", BURN_DEPENDENCY_ACTION_REGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteRollbackBoundary(pPlan, fRollback, dwIndex++, L"WixDefaultBoundary", TRUE, FALSE);
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PackageA", L"{E6469F05-BDC8-4EB8-B218-67412543EFAA}", BURN_DEPENDENCY_ACTION_REGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageProvider(pPlan, fRollback, dwIndex++, L"PackageA", BURN_DEPENDENCY_ACTION_REGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteMsiPackage(pPlan, fRollback, dwIndex++, L"PackageA", BOOTSTRAPPER_ACTION_STATE_INSTALL, BURN_MSI_PROPERTY_INSTALL, INSTALLUILEVEL_NONE, FALSE, 0);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            Assert::Equal(dwIndex, pPlan->cRollbackActions);

            Assert::Equal(3ul, pPlan->cExecutePackagesTotal);
            Assert::Equal(3ul, pPlan->cOverallProgressTicksTotal);

            dwIndex = 0;
            ValidateCleanAction(pPlan, dwIndex++, L"PackageC");
            ValidateCleanAction(pPlan, dwIndex++, L"PackageB");
            ValidateCleanAction(pPlan, dwIndex++, L"PackageA");
            Assert::Equal(dwIndex, pPlan->cCleanActions);

            UINT uIndex = 0;
            ValidatePlannedProvider(pPlan, uIndex++, L"{E6469F05-BDC8-4EB8-B218-67412543EFAA}", NULL);
            ValidatePlannedProvider(pPlan, uIndex++, L"{A497C5E5-C78B-4F0B-BF72-B33E1DB1C4B8}", NULL);
            ValidatePlannedProvider(pPlan, uIndex++, L"{D1D01094-23CE-4AF0-84B6-4A1A133F21D3}", NULL);
            ValidatePlannedProvider(pPlan, uIndex++, L"{01E6B748-7B95-4BA9-976D-B6F35076CEF4}", NULL);
            Assert::Equal(uIndex, pPlan->cPlannedProviders);

            Assert::Equal(3ul, pEngineState->packages.cPackages);
            ValidateNonPermanentPackageExpectedStates(&pEngineState->packages.rgPackages[0], L"PackageA", BURN_PACKAGE_REGISTRATION_STATE_ABSENT, BURN_PACKAGE_REGISTRATION_STATE_ABSENT);
            ValidateNonPermanentPackageExpectedStates(&pEngineState->packages.rgPackages[1], L"PackageB", BURN_PACKAGE_REGISTRATION_STATE_ABSENT, BURN_PACKAGE_REGISTRATION_STATE_ABSENT);
            ValidateNonPermanentPackageExpectedStates(&pEngineState->packages.rgPackages[2], L"PackageC", BURN_PACKAGE_REGISTRATION_STATE_ABSENT, BURN_PACKAGE_REGISTRATION_STATE_ABSENT);
        }

        [Fact]
        void RelatedBundleMissingFromCacheTest()
        {
            HRESULT hr = S_OK;
            BURN_ENGINE_STATE engineState = { };
            BURN_ENGINE_STATE* pEngineState = &engineState;
            BURN_PLAN* pPlan = &engineState.plan;

            InitializeEngineStateForCorePlan(wzSingleMsiManifestFileName, pEngineState);
            DetectAttachedContainerAsAttached(pEngineState);
            DetectPackagesAsAbsent(pEngineState);
            BURN_RELATED_BUNDLE* pRelatedBundle = DetectUpgradeBundle(pEngineState, L"{FD9920AD-DBCA-4C6C-8CD5-B47431CE8D21}", L"0.9.0.0");
            pRelatedBundle->fPlannable = FALSE;

            hr = CorePlan(pEngineState, BOOTSTRAPPER_ACTION_INSTALL);
            NativeAssert::Succeeded(hr, "CorePlan failed");

            Assert::Equal<DWORD>(BOOTSTRAPPER_ACTION_INSTALL, pPlan->action);
            Assert::Equal<BOOL>(TRUE, pPlan->fPerMachine);
            Assert::Equal<BOOL>(FALSE, pPlan->fDisableRollback);

            BOOL fRollback = FALSE;
            DWORD dwIndex = 0;
            ValidateCacheCheckpoint(pPlan, fRollback, dwIndex++, 1);
            ValidateCachePackage(pPlan, fRollback, dwIndex++, L"PackageA");
            ValidateCacheSignalSyncpoint(pPlan, fRollback, dwIndex++);
            Assert::Equal(dwIndex, pPlan->cCacheActions);

            fRollback = TRUE;
            dwIndex = 0;
            ValidateCacheCheckpoint(pPlan, fRollback, dwIndex++, 1);
            Assert::Equal(dwIndex, pPlan->cRollbackCacheActions);

            Assert::Equal(35694ull, pPlan->qwEstimatedSize);
            Assert::Equal(168715ull, pPlan->qwCacheSizeTotal);

            fRollback = FALSE;
            dwIndex = 0;
            DWORD dwExecuteCheckpointId = 2;
            ValidateExecuteRollbackBoundary(pPlan, fRollback, dwIndex++, L"WixDefaultBoundary", TRUE, FALSE);
            ValidateExecuteWaitSyncpoint(pPlan, fRollback, dwIndex++, pPlan->rgCacheActions[2].syncpoint.hEvent);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageProvider(pPlan, fRollback, dwIndex++, L"PackageA", BURN_DEPENDENCY_ACTION_REGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteMsiPackage(pPlan, fRollback, dwIndex++, L"PackageA", BOOTSTRAPPER_ACTION_STATE_INSTALL, BURN_MSI_PROPERTY_INSTALL, INSTALLUILEVEL_NONE, FALSE, 0);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PackageA", L"{A6F0CBF7-1578-450C-B9D7-9CF2EEC40002}", BURN_DEPENDENCY_ACTION_REGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            Assert::Equal(dwIndex, pPlan->cExecuteActions);

            fRollback = TRUE;
            dwIndex = 0;
            dwExecuteCheckpointId = 2;
            ValidateExecuteRollbackBoundary(pPlan, fRollback, dwIndex++, L"WixDefaultBoundary", TRUE, FALSE);
            ValidateExecuteUncachePackage(pPlan, fRollback, dwIndex++, L"PackageA");
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageProvider(pPlan, fRollback, dwIndex++, L"PackageA", BURN_DEPENDENCY_ACTION_UNREGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteMsiPackage(pPlan, fRollback, dwIndex++, L"PackageA", BOOTSTRAPPER_ACTION_STATE_UNINSTALL, BURN_MSI_PROPERTY_UNINSTALL, INSTALLUILEVEL_NONE, FALSE, 0);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PackageA", L"{A6F0CBF7-1578-450C-B9D7-9CF2EEC40002}", BURN_DEPENDENCY_ACTION_UNREGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            Assert::Equal(dwIndex, pPlan->cRollbackActions);

            Assert::Equal(1ul, pPlan->cExecutePackagesTotal);
            Assert::Equal(2ul, pPlan->cOverallProgressTicksTotal);

            dwIndex = 0;
            Assert::Equal(dwIndex, pPlan->cCleanActions);

            UINT uIndex = 0;
            ValidatePlannedProvider(pPlan, uIndex++, L"{A6F0CBF7-1578-450C-B9D7-9CF2EEC40002}", NULL);
            Assert::Equal(uIndex, pPlan->cPlannedProviders);

            Assert::Equal(1ul, pEngineState->packages.cPackages);
            ValidateNonPermanentPackageExpectedStates(&pEngineState->packages.rgPackages[0], L"PackageA", BURN_PACKAGE_REGISTRATION_STATE_PRESENT, BURN_PACKAGE_REGISTRATION_STATE_PRESENT);
        }

        [Fact]
        void SingleMsiCacheTest()
        {
            HRESULT hr = S_OK;
            BURN_ENGINE_STATE engineState = { };
            BURN_ENGINE_STATE* pEngineState = &engineState;
            BURN_PLAN* pPlan = &engineState.plan;

            InitializeEngineStateForCorePlan(wzSingleMsiManifestFileName, pEngineState);
            DetectAttachedContainerAsAttached(pEngineState);
            DetectPackagesAsAbsent(pEngineState);
            DetectUpgradeBundle(pEngineState, L"{FD9920AD-DBCA-4C6C-8CD5-B47431CE8D21}", L"0.9.0.0");

            hr = CorePlan(pEngineState, BOOTSTRAPPER_ACTION_CACHE);
            NativeAssert::Succeeded(hr, "CorePlan failed");

            Assert::Equal<DWORD>(BOOTSTRAPPER_ACTION_CACHE, pPlan->action);
            Assert::Equal<BOOL>(TRUE, pPlan->fPerMachine);
            Assert::Equal<BOOL>(FALSE, pPlan->fDisableRollback);

            BOOL fRollback = FALSE;
            DWORD dwIndex = 0;
            ValidateCacheCheckpoint(pPlan, fRollback, dwIndex++, 1);
            ValidateCachePackage(pPlan, fRollback, dwIndex++, L"PackageA");
            ValidateCacheSignalSyncpoint(pPlan, fRollback, dwIndex++);
            Assert::Equal(dwIndex, pPlan->cCacheActions);

            fRollback = TRUE;
            dwIndex = 0;
            Assert::Equal(dwIndex, pPlan->cRollbackCacheActions);

            Assert::Equal(33743ull, pPlan->qwEstimatedSize);
            Assert::Equal(168715ull, pPlan->qwCacheSizeTotal);

            fRollback = FALSE;
            dwIndex = 0;
            DWORD dwExecuteCheckpointId = 2;
            ValidateExecuteRollbackBoundary(pPlan, fRollback, dwIndex++, L"WixDefaultBoundary", TRUE, FALSE);
            ValidateExecuteWaitSyncpoint(pPlan, fRollback, dwIndex++, pPlan->rgCacheActions[2].syncpoint.hEvent);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            Assert::Equal(dwIndex, pPlan->cExecuteActions);

            fRollback = TRUE;
            dwIndex = 0;
            dwExecuteCheckpointId = 2;
            ValidateExecuteRollbackBoundary(pPlan, fRollback, dwIndex++, L"WixDefaultBoundary", TRUE, FALSE);
            ValidateExecuteUncachePackage(pPlan, fRollback, dwIndex++, L"PackageA");
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageProvider(pPlan, fRollback, dwIndex++, L"PackageA", BURN_DEPENDENCY_ACTION_UNREGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            Assert::Equal(dwIndex, pPlan->cRollbackActions);

            Assert::Equal(0ul, pPlan->cExecutePackagesTotal);
            Assert::Equal(1ul, pPlan->cOverallProgressTicksTotal);

            dwIndex = 0;
            Assert::Equal(dwIndex, pPlan->cCleanActions);

            UINT uIndex = 0;
            ValidatePlannedProvider(pPlan, uIndex++, L"{A6F0CBF7-1578-450C-B9D7-9CF2EEC40002}", NULL);
            Assert::Equal(uIndex, pPlan->cPlannedProviders);

            Assert::Equal(1ul, pEngineState->packages.cPackages);
            ValidateNonPermanentPackageExpectedStates(&pEngineState->packages.rgPackages[0], L"PackageA", BURN_PACKAGE_REGISTRATION_STATE_PRESENT, BURN_PACKAGE_REGISTRATION_STATE_ABSENT);
        }

        [Fact]
        void SingleMsiInstallTest()
        {
            HRESULT hr = S_OK;
            BURN_ENGINE_STATE engineState = { };
            BURN_ENGINE_STATE* pEngineState = &engineState;
            BURN_PLAN* pPlan = &engineState.plan;

            InitializeEngineStateForCorePlan(wzSingleMsiManifestFileName, pEngineState);
            DetectAttachedContainerAsAttached(pEngineState);
            DetectPackagesAsAbsent(pEngineState);
            DetectUpgradeBundle(pEngineState, L"{FD9920AD-DBCA-4C6C-8CD5-B47431CE8D21}", L"0.9.0.0");

            hr = CorePlan(pEngineState, BOOTSTRAPPER_ACTION_INSTALL);
            NativeAssert::Succeeded(hr, "CorePlan failed");

            Assert::Equal<DWORD>(BOOTSTRAPPER_ACTION_INSTALL, pPlan->action);
            Assert::Equal<BOOL>(TRUE, pPlan->fPerMachine);
            Assert::Equal<BOOL>(FALSE, pPlan->fDisableRollback);

            BOOL fRollback = FALSE;
            DWORD dwIndex = 0;
            ValidateCacheCheckpoint(pPlan, fRollback, dwIndex++, 1);
            ValidateCachePackage(pPlan, fRollback, dwIndex++, L"PackageA");
            ValidateCacheSignalSyncpoint(pPlan, fRollback, dwIndex++);
            Assert::Equal(dwIndex, pPlan->cCacheActions);

            fRollback = TRUE;
            dwIndex = 0;
            ValidateCacheCheckpoint(pPlan, fRollback, dwIndex++, 1);
            Assert::Equal(dwIndex, pPlan->cRollbackCacheActions);

            Assert::Equal(35694ull, pPlan->qwEstimatedSize);
            Assert::Equal(168715ull, pPlan->qwCacheSizeTotal);

            fRollback = FALSE;
            dwIndex = 0;
            DWORD dwExecuteCheckpointId = 2;
            ValidateExecuteRollbackBoundary(pPlan, fRollback, dwIndex++, L"WixDefaultBoundary", TRUE, FALSE);
            ValidateExecuteWaitSyncpoint(pPlan, fRollback, dwIndex++, pPlan->rgCacheActions[2].syncpoint.hEvent);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageProvider(pPlan, fRollback, dwIndex++, L"PackageA", BURN_DEPENDENCY_ACTION_REGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteMsiPackage(pPlan, fRollback, dwIndex++, L"PackageA", BOOTSTRAPPER_ACTION_STATE_INSTALL, BURN_MSI_PROPERTY_INSTALL, INSTALLUILEVEL_NONE, FALSE, 0);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PackageA", L"{A6F0CBF7-1578-450C-B9D7-9CF2EEC40002}", BURN_DEPENDENCY_ACTION_REGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteExePackage(pPlan, fRollback, dwIndex++, L"{FD9920AD-DBCA-4C6C-8CD5-B47431CE8D21}", BOOTSTRAPPER_ACTION_STATE_UNINSTALL, NULL);
            Assert::Equal(dwIndex, pPlan->cExecuteActions);

            fRollback = TRUE;
            dwIndex = 0;
            dwExecuteCheckpointId = 2;
            ValidateExecuteRollbackBoundary(pPlan, fRollback, dwIndex++, L"WixDefaultBoundary", TRUE, FALSE);
            ValidateExecuteUncachePackage(pPlan, fRollback, dwIndex++, L"PackageA");
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageProvider(pPlan, fRollback, dwIndex++, L"PackageA", BURN_DEPENDENCY_ACTION_UNREGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteMsiPackage(pPlan, fRollback, dwIndex++, L"PackageA", BOOTSTRAPPER_ACTION_STATE_UNINSTALL, BURN_MSI_PROPERTY_UNINSTALL, INSTALLUILEVEL_NONE, FALSE, 0);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PackageA", L"{A6F0CBF7-1578-450C-B9D7-9CF2EEC40002}", BURN_DEPENDENCY_ACTION_UNREGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteExePackage(pPlan, fRollback, dwIndex++, L"{FD9920AD-DBCA-4C6C-8CD5-B47431CE8D21}", BOOTSTRAPPER_ACTION_STATE_INSTALL, NULL);
            Assert::Equal(dwIndex, pPlan->cRollbackActions);

            Assert::Equal(2ul, pPlan->cExecutePackagesTotal);
            Assert::Equal(3ul, pPlan->cOverallProgressTicksTotal);

            dwIndex = 0;
            Assert::Equal(dwIndex, pPlan->cCleanActions);

            UINT uIndex = 0;
            ValidatePlannedProvider(pPlan, uIndex++, L"{A6F0CBF7-1578-450C-B9D7-9CF2EEC40002}", NULL);
            Assert::Equal(uIndex, pPlan->cPlannedProviders);

            Assert::Equal(1ul, pEngineState->packages.cPackages);
            ValidateNonPermanentPackageExpectedStates(&pEngineState->packages.rgPackages[0], L"PackageA", BURN_PACKAGE_REGISTRATION_STATE_PRESENT, BURN_PACKAGE_REGISTRATION_STATE_PRESENT);
        }

        [Fact]
        void SingleMsiInstalledWithNoInstalledPackagesModifyTest()
        {
            HRESULT hr = S_OK;
            BURN_ENGINE_STATE engineState = { };
            BURN_ENGINE_STATE* pEngineState = &engineState;
            BURN_PLAN* pPlan = &engineState.plan;

            InitializeEngineStateForCorePlan(wzSingleMsiManifestFileName, pEngineState);
            DetectPackagesAsAbsent(pEngineState);

            pEngineState->registration.fInstalled = TRUE;

            hr = CorePlan(pEngineState, BOOTSTRAPPER_ACTION_MODIFY);
            NativeAssert::Succeeded(hr, "CorePlan failed");

            Assert::Equal<DWORD>(BOOTSTRAPPER_ACTION_MODIFY, pPlan->action);
            Assert::Equal<BOOL>(TRUE, pPlan->fPerMachine);
            Assert::Equal<BOOL>(FALSE, pPlan->fDisableRollback);

            BOOL fRollback = FALSE;
            DWORD dwIndex = 0;
            Assert::Equal(dwIndex, pPlan->cCacheActions);

            fRollback = TRUE;
            dwIndex = 0;
            Assert::Equal(dwIndex, pPlan->cRollbackCacheActions);

            Assert::Equal(0ull, pPlan->qwEstimatedSize);
            Assert::Equal(0ull, pPlan->qwCacheSizeTotal);

            fRollback = FALSE;
            dwIndex = 0;
            DWORD dwExecuteCheckpointId = 1;
            ValidateExecuteRollbackBoundary(pPlan, fRollback, dwIndex++, L"WixDefaultBoundary", TRUE, FALSE);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            Assert::Equal(dwIndex, pPlan->cExecuteActions);

            fRollback = TRUE;
            dwIndex = 0;
            dwExecuteCheckpointId = 1;
            ValidateExecuteRollbackBoundary(pPlan, fRollback, dwIndex++, L"WixDefaultBoundary", TRUE, FALSE);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            Assert::Equal(dwIndex, pPlan->cRollbackActions);

            Assert::Equal(0ul, pPlan->cExecutePackagesTotal);
            Assert::Equal(0ul, pPlan->cOverallProgressTicksTotal);

            dwIndex = 0;
            ValidateCleanAction(pPlan, dwIndex++, L"PackageA");
            Assert::Equal(dwIndex, pPlan->cCleanActions);

            UINT uIndex = 0;
            ValidatePlannedProvider(pPlan, uIndex++, L"{A6F0CBF7-1578-450C-B9D7-9CF2EEC40002}", NULL);
            Assert::Equal(uIndex, pPlan->cPlannedProviders);

            Assert::Equal(1ul, pEngineState->packages.cPackages);
            ValidateNonPermanentPackageExpectedStates(&pEngineState->packages.rgPackages[0], L"PackageA", BURN_PACKAGE_REGISTRATION_STATE_ABSENT, BURN_PACKAGE_REGISTRATION_STATE_ABSENT);
        }

        [Fact]
        void SingleMsiUninstallTest()
        {
            HRESULT hr = S_OK;
            BURN_ENGINE_STATE engineState = { };
            BURN_ENGINE_STATE* pEngineState = &engineState;
            BURN_PLAN* pPlan = &engineState.plan;

            InitializeEngineStateForCorePlan(wzSingleMsiManifestFileName, pEngineState);
            DetectPackagesAsPresentAndCached(pEngineState);

            hr = CorePlan(pEngineState, BOOTSTRAPPER_ACTION_UNINSTALL);
            NativeAssert::Succeeded(hr, "CorePlan failed");

            Assert::Equal<DWORD>(BOOTSTRAPPER_ACTION_UNINSTALL, pPlan->action);
            Assert::Equal<BOOL>(TRUE, pPlan->fPerMachine);
            Assert::Equal<BOOL>(FALSE, pPlan->fDisableRollback);

            BOOL fRollback = FALSE;
            DWORD dwIndex = 0;
            Assert::Equal(dwIndex, pPlan->cCacheActions);

            fRollback = TRUE;
            dwIndex = 0;
            Assert::Equal(dwIndex, pPlan->cRollbackCacheActions);

            Assert::Equal(0ull, pPlan->qwEstimatedSize);
            Assert::Equal(0ull, pPlan->qwCacheSizeTotal);

            fRollback = FALSE;
            dwIndex = 0;
            DWORD dwExecuteCheckpointId = 1;
            ValidateExecuteRollbackBoundary(pPlan, fRollback, dwIndex++, L"WixDefaultBoundary", TRUE, FALSE);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PackageA", L"{A6F0CBF7-1578-450C-B9D7-9CF2EEC40002}", BURN_DEPENDENCY_ACTION_UNREGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageProvider(pPlan, fRollback, dwIndex++, L"PackageA", BURN_DEPENDENCY_ACTION_UNREGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteMsiPackage(pPlan, fRollback, dwIndex++, L"PackageA", BOOTSTRAPPER_ACTION_STATE_UNINSTALL, BURN_MSI_PROPERTY_UNINSTALL, INSTALLUILEVEL_NONE, FALSE, 0);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            Assert::Equal(dwIndex, pPlan->cExecuteActions);

            fRollback = TRUE;
            dwIndex = 0;
            dwExecuteCheckpointId = 1;
            ValidateExecuteRollbackBoundary(pPlan, fRollback, dwIndex++, L"WixDefaultBoundary", TRUE, FALSE);
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PackageA", L"{A6F0CBF7-1578-450C-B9D7-9CF2EEC40002}", BURN_DEPENDENCY_ACTION_REGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageProvider(pPlan, fRollback, dwIndex++, L"PackageA", BURN_DEPENDENCY_ACTION_REGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteMsiPackage(pPlan, fRollback, dwIndex++, L"PackageA", BOOTSTRAPPER_ACTION_STATE_INSTALL, BURN_MSI_PROPERTY_INSTALL, INSTALLUILEVEL_NONE, FALSE, 0);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            Assert::Equal(dwIndex, pPlan->cRollbackActions);

            Assert::Equal(1ul, pPlan->cExecutePackagesTotal);
            Assert::Equal(1ul, pPlan->cOverallProgressTicksTotal);

            dwIndex = 0;
            ValidateCleanAction(pPlan, dwIndex++, L"PackageA");
            Assert::Equal(dwIndex, pPlan->cCleanActions);

            UINT uIndex = 0;
            ValidatePlannedProvider(pPlan, uIndex++, L"{A6F0CBF7-1578-450C-B9D7-9CF2EEC40002}", NULL);
            ValidatePlannedProvider(pPlan, uIndex++, L"{64633047-D172-4BBB-B202-64337D15C952}", NULL);
            Assert::Equal(uIndex, pPlan->cPlannedProviders);

            Assert::Equal(1ul, pEngineState->packages.cPackages);
            ValidateNonPermanentPackageExpectedStates(&pEngineState->packages.rgPackages[0], L"PackageA", BURN_PACKAGE_REGISTRATION_STATE_ABSENT, BURN_PACKAGE_REGISTRATION_STATE_ABSENT);
        }

        [Fact]
        void SingleMsiUninstallTestFromUpgradeBundleWithSameExactPackage()
        {
            HRESULT hr = S_OK;
            BURN_ENGINE_STATE engineState = { };
            BURN_ENGINE_STATE* pEngineState = &engineState;
            BURN_PLAN* pPlan = &engineState.plan;

            InitializeEngineStateForCorePlan(wzSingleMsiManifestFileName, pEngineState);
            DetectAsRelatedUpgradeBundle(&engineState, L"{02940F3E-C83E-452D-BFCF-C943777ACEAE}", L"2.0.0.0");

            hr = CorePlan(pEngineState, BOOTSTRAPPER_ACTION_UNINSTALL);
            NativeAssert::Succeeded(hr, "CorePlan failed");

            Assert::Equal<DWORD>(BOOTSTRAPPER_ACTION_UNINSTALL, pPlan->action);
            Assert::Equal<BOOL>(TRUE, pPlan->fPerMachine);
            Assert::Equal<BOOL>(FALSE, pPlan->fDisableRollback);

            BOOL fRollback = FALSE;
            DWORD dwIndex = 0;
            Assert::Equal(dwIndex, pPlan->cCacheActions);

            fRollback = TRUE;
            dwIndex = 0;
            Assert::Equal(dwIndex, pPlan->cRollbackCacheActions);

            Assert::Equal(0ull, pPlan->qwEstimatedSize);
            Assert::Equal(0ull, pPlan->qwCacheSizeTotal);

            fRollback = FALSE;
            dwIndex = 0;
            DWORD dwExecuteCheckpointId = 1;
            ValidateExecuteRollbackBoundary(pPlan, fRollback, dwIndex++, L"WixDefaultBoundary", TRUE, FALSE);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PackageA", L"{A6F0CBF7-1578-450C-B9D7-9CF2EEC40002}", BURN_DEPENDENCY_ACTION_UNREGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            Assert::Equal(dwIndex, pPlan->cExecuteActions);

            fRollback = TRUE;
            dwIndex = 0;
            dwExecuteCheckpointId = 1;
            ValidateExecuteRollbackBoundary(pPlan, fRollback, dwIndex++, L"WixDefaultBoundary", TRUE, FALSE);
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PackageA", L"{A6F0CBF7-1578-450C-B9D7-9CF2EEC40002}", BURN_DEPENDENCY_ACTION_REGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            Assert::Equal(dwIndex, pPlan->cRollbackActions);

            Assert::Equal(0ul, pPlan->cExecutePackagesTotal);
            Assert::Equal(0ul, pPlan->cOverallProgressTicksTotal);

            dwIndex = 0;
            Assert::Equal(dwIndex, pPlan->cCleanActions);

            UINT uIndex = 0;
            ValidatePlannedProvider(pPlan, uIndex++, L"{A6F0CBF7-1578-450C-B9D7-9CF2EEC40002}", NULL);
            Assert::Equal(uIndex, pPlan->cPlannedProviders);

            Assert::Equal(1ul, pEngineState->packages.cPackages);
            ValidateNonPermanentPackageExpectedStates(&pEngineState->packages.rgPackages[0], L"PackageA", BURN_PACKAGE_REGISTRATION_STATE_IGNORED, BURN_PACKAGE_REGISTRATION_STATE_IGNORED);
        }

        [Fact]
        void SlipstreamInstallTest()
        {
            HRESULT hr = S_OK;
            BURN_ENGINE_STATE engineState = { };
            BURN_ENGINE_STATE* pEngineState = &engineState;
            BURN_PLAN* pPlan = &engineState.plan;

            InitializeEngineStateForCorePlan(wzSlipstreamManifestFileName, pEngineState);
            DetectPermanentPackagesAsPresentAndCached(pEngineState);
            PlanTestDetectPatchInitialize(pEngineState);

            hr = CorePlan(pEngineState, BOOTSTRAPPER_ACTION_INSTALL);
            NativeAssert::Succeeded(hr, "CorePlan failed");

            Assert::Equal<DWORD>(BOOTSTRAPPER_ACTION_INSTALL, pPlan->action);
            Assert::Equal<BOOL>(TRUE, pPlan->fPerMachine);
            Assert::Equal<BOOL>(FALSE, pPlan->fDisableRollback);

            BOOL fRollback = FALSE;
            DWORD dwIndex = 0;
            ValidateCacheCheckpoint(pPlan, fRollback, dwIndex++, 1);
            ValidateCachePackage(pPlan, fRollback, dwIndex++, L"PatchA");
            ValidateCacheSignalSyncpoint(pPlan, fRollback, dwIndex++);
            ValidateCacheCheckpoint(pPlan, fRollback, dwIndex++, 2);
            ValidateCachePackage(pPlan, fRollback, dwIndex++, L"PackageA");
            ValidateCacheSignalSyncpoint(pPlan, fRollback, dwIndex++);
            Assert::Equal(dwIndex, pPlan->cCacheActions);

            fRollback = TRUE;
            dwIndex = 0;
            ValidateCacheCheckpoint(pPlan, fRollback, dwIndex++, 2);
            Assert::Equal(dwIndex, pPlan->cRollbackCacheActions);

            Assert::Equal(3055111ull, pPlan->qwEstimatedSize);
            Assert::Equal(212992ull, pPlan->qwCacheSizeTotal);

            fRollback = FALSE;
            dwIndex = 0;
            DWORD dwExecuteCheckpointId = 3;
            BURN_EXECUTE_ACTION* pExecuteAction = NULL;
            ValidateExecuteRollbackBoundary(pPlan, fRollback, dwIndex++, L"WixDefaultBoundary", TRUE, FALSE);
            ValidateExecuteWaitSyncpoint(pPlan, fRollback, dwIndex++, pPlan->rgCacheActions[5].syncpoint.hEvent);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageProvider(pPlan, fRollback, dwIndex++, L"PackageA", BURN_DEPENDENCY_ACTION_REGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteMsiPackage(pPlan, fRollback, dwIndex++, L"PackageA", BOOTSTRAPPER_ACTION_STATE_INSTALL, BURN_MSI_PROPERTY_INSTALL, INSTALLUILEVEL_NONE, FALSE, 0);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PackageA", L"{22D1DDBA-284D-40A7-BD14-95EA07906F21}", BURN_DEPENDENCY_ACTION_REGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteWaitSyncpoint(pPlan, fRollback, dwIndex++, pPlan->rgCacheActions[2].syncpoint.hEvent);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageProvider(pPlan, fRollback, dwIndex++, L"PatchA", BURN_DEPENDENCY_ACTION_REGISTER);
            pExecuteAction = ValidateDeletedExecuteMspTarget(pPlan, fRollback, dwIndex++, L"PatchA", BOOTSTRAPPER_ACTION_STATE_INSTALL, L"{5FF7F534-3FFC-41E0-80CD-E6361E5E7B7B}", TRUE, BURN_MSI_PROPERTY_INSTALL, INSTALLUILEVEL_NONE, FALSE, TRUE);
            ValidateExecuteMspTargetPatch(pExecuteAction, 0, L"PatchA");
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PatchA", L"{22D1DDBA-284D-40A7-BD14-95EA07906F21}", BURN_DEPENDENCY_ACTION_REGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            Assert::Equal(dwIndex, pPlan->cExecuteActions);

            fRollback = TRUE;
            dwIndex = 0;
            dwExecuteCheckpointId = 3;
            ValidateExecuteRollbackBoundary(pPlan, fRollback, dwIndex++, L"WixDefaultBoundary", TRUE, FALSE);
            ValidateExecuteUncachePackage(pPlan, fRollback, dwIndex++, L"PackageA");
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageProvider(pPlan, fRollback, dwIndex++, L"PackageA", BURN_DEPENDENCY_ACTION_UNREGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteMsiPackage(pPlan, fRollback, dwIndex++, L"PackageA", BOOTSTRAPPER_ACTION_STATE_UNINSTALL, BURN_MSI_PROPERTY_UNINSTALL, INSTALLUILEVEL_NONE, FALSE, 0);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PackageA", L"{22D1DDBA-284D-40A7-BD14-95EA07906F21}", BURN_DEPENDENCY_ACTION_UNREGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteUncachePackage(pPlan, fRollback, dwIndex++, L"PatchA");
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageProvider(pPlan, fRollback, dwIndex++, L"PatchA", BURN_DEPENDENCY_ACTION_UNREGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            pExecuteAction = ValidateDeletedExecuteMspTarget(pPlan, fRollback, dwIndex++, L"PatchA", BOOTSTRAPPER_ACTION_STATE_UNINSTALL, L"{5FF7F534-3FFC-41E0-80CD-E6361E5E7B7B}", TRUE, BURN_MSI_PROPERTY_UNINSTALL, INSTALLUILEVEL_NONE, FALSE, TRUE);
            ValidateExecuteMspTargetPatch(pExecuteAction, 0, L"PatchA");
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PatchA", L"{22D1DDBA-284D-40A7-BD14-95EA07906F21}", BURN_DEPENDENCY_ACTION_UNREGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            Assert::Equal(dwIndex, pPlan->cRollbackActions);

            Assert::Equal(2ul, pPlan->cExecutePackagesTotal);
            Assert::Equal(4ul, pPlan->cOverallProgressTicksTotal);

            dwIndex = 0;
            Assert::Equal(dwIndex, pPlan->cCleanActions);

            UINT uIndex = 0;
            ValidatePlannedProvider(pPlan, uIndex++, L"{22D1DDBA-284D-40A7-BD14-95EA07906F21}", NULL);
            Assert::Equal(uIndex, pPlan->cPlannedProviders);

            Assert::Equal(3ul, pEngineState->packages.cPackages);
            ValidatePermanentPackageExpectedStates(&pEngineState->packages.rgPackages[0], L"NetFx48Web");
            ValidateNonPermanentPackageExpectedStates(&pEngineState->packages.rgPackages[1], L"PackageA", BURN_PACKAGE_REGISTRATION_STATE_PRESENT, BURN_PACKAGE_REGISTRATION_STATE_PRESENT);
            ValidateNonPermanentPackageExpectedStates(&pEngineState->packages.rgPackages[2], L"PatchA", BURN_PACKAGE_REGISTRATION_STATE_PRESENT, BURN_PACKAGE_REGISTRATION_STATE_PRESENT);
        }

        [Fact]
        void SlipstreamUninstallTest()
        {
            HRESULT hr = S_OK;
            BURN_ENGINE_STATE engineState = { };
            BURN_ENGINE_STATE* pEngineState = &engineState;
            BURN_PLAN* pPlan = &engineState.plan;

            InitializeEngineStateForCorePlan(wzSlipstreamManifestFileName, pEngineState);
            DetectPackagesAsPresentAndCached(pEngineState);

            hr = CorePlan(pEngineState, BOOTSTRAPPER_ACTION_UNINSTALL);
            NativeAssert::Succeeded(hr, "CorePlan failed");

            Assert::Equal<DWORD>(BOOTSTRAPPER_ACTION_UNINSTALL, pPlan->action);
            Assert::Equal<BOOL>(TRUE, pPlan->fPerMachine);
            Assert::Equal<BOOL>(FALSE, pPlan->fDisableRollback);

            BOOL fRollback = FALSE;
            DWORD dwIndex = 0;
            Assert::Equal(dwIndex, pPlan->cCacheActions);

            fRollback = TRUE;
            dwIndex = 0;
            Assert::Equal(dwIndex, pPlan->cRollbackCacheActions);

            Assert::Equal(0ull, pPlan->qwEstimatedSize);
            Assert::Equal(0ull, pPlan->qwCacheSizeTotal);

            fRollback = FALSE;
            dwIndex = 0;
            DWORD dwExecuteCheckpointId = 1;
            BURN_EXECUTE_ACTION* pExecuteAction = NULL;
            ValidateExecuteRollbackBoundary(pPlan, fRollback, dwIndex++, L"WixDefaultBoundary", TRUE, FALSE);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PatchA", L"{22D1DDBA-284D-40A7-BD14-95EA07906F21}", BURN_DEPENDENCY_ACTION_UNREGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageProvider(pPlan, fRollback, dwIndex++, L"PatchA", BURN_DEPENDENCY_ACTION_UNREGISTER);
            pExecuteAction = ValidateDeletedExecuteMspTarget(pPlan, fRollback, dwIndex++, L"PatchA", BOOTSTRAPPER_ACTION_STATE_UNINSTALL, L"{5FF7F534-3FFC-41E0-80CD-E6361E5E7B7B}", TRUE, BURN_MSI_PROPERTY_UNINSTALL, INSTALLUILEVEL_NONE, FALSE, TRUE);
            ValidateExecuteMspTargetPatch(pExecuteAction, 0, L"PatchA");
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PackageA", L"{22D1DDBA-284D-40A7-BD14-95EA07906F21}", BURN_DEPENDENCY_ACTION_UNREGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageProvider(pPlan, fRollback, dwIndex++, L"PackageA", BURN_DEPENDENCY_ACTION_UNREGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteMsiPackage(pPlan, fRollback, dwIndex++, L"PackageA", BOOTSTRAPPER_ACTION_STATE_UNINSTALL, BURN_MSI_PROPERTY_UNINSTALL, INSTALLUILEVEL_NONE, FALSE, 0);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            Assert::Equal(dwIndex, pPlan->cExecuteActions);

            fRollback = TRUE;
            dwIndex = 0;
            dwExecuteCheckpointId = 1;
            ValidateExecuteRollbackBoundary(pPlan, fRollback, dwIndex++, L"WixDefaultBoundary", TRUE, FALSE);
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PatchA", L"{22D1DDBA-284D-40A7-BD14-95EA07906F21}", BURN_DEPENDENCY_ACTION_REGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageProvider(pPlan, fRollback, dwIndex++, L"PatchA", BURN_DEPENDENCY_ACTION_REGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            pExecuteAction = ValidateDeletedExecuteMspTarget(pPlan, fRollback, dwIndex++, L"PatchA", BOOTSTRAPPER_ACTION_STATE_INSTALL, L"{5FF7F534-3FFC-41E0-80CD-E6361E5E7B7B}", TRUE, BURN_MSI_PROPERTY_INSTALL, INSTALLUILEVEL_NONE, FALSE, TRUE);
            ValidateExecuteMspTargetPatch(pExecuteAction, 0, L"PatchA");
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageDependency(pPlan, fRollback, dwIndex++, L"PackageA", L"{22D1DDBA-284D-40A7-BD14-95EA07906F21}", BURN_DEPENDENCY_ACTION_REGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecutePackageProvider(pPlan, fRollback, dwIndex++, L"PackageA", BURN_DEPENDENCY_ACTION_REGISTER);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteMsiPackage(pPlan, fRollback, dwIndex++, L"PackageA", BOOTSTRAPPER_ACTION_STATE_INSTALL, BURN_MSI_PROPERTY_INSTALL, INSTALLUILEVEL_NONE, FALSE, 0);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            ValidateExecuteCheckpoint(pPlan, fRollback, dwIndex++, dwExecuteCheckpointId++);
            Assert::Equal(dwIndex, pPlan->cRollbackActions);

            Assert::Equal(2ul, pPlan->cExecutePackagesTotal);
            Assert::Equal(2ul, pPlan->cOverallProgressTicksTotal);

            dwIndex = 0;
            ValidateCleanAction(pPlan, dwIndex++, L"PatchA");
            ValidateCleanAction(pPlan, dwIndex++, L"PackageA");
            Assert::Equal(dwIndex, pPlan->cCleanActions);

            UINT uIndex = 0;
            ValidatePlannedProvider(pPlan, uIndex++, L"{22D1DDBA-284D-40A7-BD14-95EA07906F21}", NULL);
            ValidatePlannedProvider(pPlan, uIndex++, L"{0A5113E3-06A5-4CE0-8E83-9EB42F6764A6}", NULL);
            ValidatePlannedProvider(pPlan, uIndex++, L"{5FF7F534-3FFC-41E0-80CD-E6361E5E7B7B}", NULL);
            Assert::Equal(uIndex, pPlan->cPlannedProviders);

            Assert::Equal(3ul, pEngineState->packages.cPackages);
            ValidatePermanentPackageExpectedStates(&pEngineState->packages.rgPackages[0], L"NetFx48Web");
            ValidateNonPermanentPackageExpectedStates(&pEngineState->packages.rgPackages[1], L"PackageA", BURN_PACKAGE_REGISTRATION_STATE_ABSENT, BURN_PACKAGE_REGISTRATION_STATE_ABSENT);
            ValidateNonPermanentPackageExpectedStates(&pEngineState->packages.rgPackages[2], L"PatchA", BURN_PACKAGE_REGISTRATION_STATE_ABSENT, BURN_PACKAGE_REGISTRATION_STATE_ABSENT);
        }

    private:
        // This doesn't initialize everything, just enough for CorePlan to work.
        void InitializeEngineStateForCorePlan(LPCWSTR wzManifestFileName, BURN_ENGINE_STATE* pEngineState)
        {
            HRESULT hr = S_OK;
            LPWSTR sczFilePath = NULL;

            ::InitializeCriticalSection(&pEngineState->userExperience.csEngineActive);

            hr = WiuInitialize();
            NativeAssert::Succeeded(hr, "Failed to initliaze wiutil.");

            hr = VariableInitialize(&pEngineState->variables);
            NativeAssert::Succeeded(hr, "Failed to initialize variables.");

            try
            {
                pin_ptr<const wchar_t> dataDirectory = PtrToStringChars(this->TestContext->TestDirectory);
                hr = PathConcat(dataDirectory, L"TestData\\PlanTest", &sczFilePath);
                NativeAssert::Succeeded(hr, "Failed to get path to test file directory.");
                hr = PathConcat(sczFilePath, wzManifestFileName, &sczFilePath);
                NativeAssert::Succeeded(hr, "Failed to get path to test file.");
                Assert::True(FileExistsEx(sczFilePath, NULL), "Test file does not exist.");

                hr = ManifestLoadXmlFromFile(sczFilePath, pEngineState);
                NativeAssert::Succeeded(hr, "Failed to load manifest.");
            }
            finally
            {
                ReleaseStr(sczFilePath);
            }

            hr = CoreInitializeConstants(pEngineState);
            NativeAssert::Succeeded(hr, "Failed to initialize core constants");

            pEngineState->userExperience.pfnBAProc = PlanTestBAProc;
        }

        void PlanTestDetect(BURN_ENGINE_STATE* pEngineState)
        {
            HRESULT hr = S_OK;
            BURN_REGISTRATION* pRegistration = &pEngineState->registration;

            DetectReset(pRegistration, &pEngineState->packages);
            PlanReset(&pEngineState->plan, &pEngineState->containers, &pEngineState->packages, &pEngineState->layoutPayloads);

            hr = DepDependencyArrayAlloc(&pRegistration->rgIgnoredDependencies, &pRegistration->cIgnoredDependencies, pRegistration->sczProviderKey, NULL);
            NativeAssert::Succeeded(hr, "Failed to add the bundle provider key to the list of dependencies to ignore.");

            pEngineState->userExperience.fEngineActive = TRUE;
            pEngineState->fDetected = TRUE;
        }

        void PlanTestDetectPatchInitialize(BURN_ENGINE_STATE* pEngineState)
        {
            HRESULT hr = MsiEngineDetectInitialize(&pEngineState->packages);
            NativeAssert::Succeeded(hr, "MsiEngineDetectInitialize failed");

            for (DWORD i = 0; i < pEngineState->packages.cPackages; ++i)
            {
                BURN_PACKAGE* pPackage = pEngineState->packages.rgPackages + i;

                if (BURN_PACKAGE_TYPE_MSP == pPackage->type)
                {
                    for (DWORD j = 0; j < pPackage->Msp.cTargetProductCodes; ++j)
                    {
                        BURN_MSPTARGETPRODUCT* pTargetProduct = pPackage->Msp.rgTargetProducts + j;

                        if (BOOTSTRAPPER_PACKAGE_STATE_UNKNOWN == pTargetProduct->patchPackageState)
                        {
                            pTargetProduct->patchPackageState = BOOTSTRAPPER_PACKAGE_STATE_ABSENT;
                        }
                    }
                }
            }
        }

        void DetectAttachedContainerAsAttached(BURN_ENGINE_STATE* pEngineState)
        {
            for (DWORD i = 0; i < pEngineState->containers.cContainers; ++i)
            {
                BURN_CONTAINER* pContainer = pEngineState->containers.rgContainers + i;
                if (pContainer->fAttached)
                {
                    pContainer->fActuallyAttached = TRUE;
                }
            }
        }

        void DetectPackageAsAbsent(BURN_PACKAGE* pPackage)
        {
            pPackage->currentState = BOOTSTRAPPER_PACKAGE_STATE_ABSENT;
            if (pPackage->fCanAffectRegistration)
            {
                pPackage->cacheRegistrationState = BURN_PACKAGE_REGISTRATION_STATE_ABSENT;
                pPackage->installRegistrationState = BURN_PACKAGE_REGISTRATION_STATE_ABSENT;
            }
        }

        void DetectPackageAsPresentAndCached(BURN_PACKAGE* pPackage)
        {
            pPackage->currentState = BOOTSTRAPPER_PACKAGE_STATE_PRESENT;
            pPackage->fCached = TRUE;
            if (pPackage->fCanAffectRegistration)
            {
                pPackage->cacheRegistrationState = BURN_PACKAGE_REGISTRATION_STATE_PRESENT;
                pPackage->installRegistrationState = BURN_PACKAGE_REGISTRATION_STATE_PRESENT;
            }
        }

        void DetectPackageDependent(BURN_PACKAGE* pPackage, LPCWSTR wzId)
        {
            HRESULT hr = S_OK;

            for (DWORD i = 0; i < pPackage->cDependencyProviders; ++i)
            {
                BURN_DEPENDENCY_PROVIDER* pProvider = pPackage->rgDependencyProviders + i;

                hr = DepDependencyArrayAlloc(&pProvider->rgDependents, &pProvider->cDependents, wzId, NULL);
                NativeAssert::Succeeded(hr, "Failed to add package dependent");
            }
        }

        void DetectPackagesAsAbsent(BURN_ENGINE_STATE* pEngineState)
        {
            PlanTestDetect(pEngineState);

            for (DWORD i = 0; i < pEngineState->packages.cPackages; ++i)
            {
                BURN_PACKAGE* pPackage = pEngineState->packages.rgPackages + i;
                DetectPackageAsAbsent(pPackage);
            }
        }

        void DetectPackagesAsPresentAndCached(BURN_ENGINE_STATE* pEngineState)
        {
            PlanTestDetect(pEngineState);

            pEngineState->registration.fInstalled = TRUE;

            for (DWORD i = 0; i < pEngineState->packages.cPackages; ++i)
            {
                BURN_PACKAGE* pPackage = pEngineState->packages.rgPackages + i;
                DetectPackageAsPresentAndCached(pPackage);
                DetectPackageDependent(pPackage, pEngineState->registration.sczId);

                if (BURN_PACKAGE_TYPE_MSI == pPackage->type)
                {
                    for (DWORD j = 0; j < pPackage->Msi.cSlipstreamMspPackages; ++j)
                    {
                        pPackage->currentState = BOOTSTRAPPER_PACKAGE_STATE_SUPERSEDED;

                        BURN_PACKAGE* pMspPackage = pPackage->Msi.rgSlipstreamMsps[j].pMspPackage;
                        MspEngineAddDetectedTargetProduct(&pEngineState->packages, pMspPackage, j, pPackage->Msi.sczProductCode, pPackage->fPerMachine ? MSIINSTALLCONTEXT_MACHINE : MSIINSTALLCONTEXT_USERUNMANAGED);

                        BURN_MSPTARGETPRODUCT* pTargetProduct = pMspPackage->Msp.rgTargetProducts + (pMspPackage->Msp.cTargetProductCodes - 1);
                        pTargetProduct->patchPackageState = BOOTSTRAPPER_PACKAGE_STATE_PRESENT;
                        pTargetProduct->registrationState = BURN_PACKAGE_REGISTRATION_STATE_PRESENT;
                    }
                }
            }
        }

        void DetectPermanentPackagesAsPresentAndCached(BURN_ENGINE_STATE* pEngineState)
        {
            PlanTestDetect(pEngineState);

            pEngineState->registration.fInstalled = TRUE;

            for (DWORD i = 0; i < pEngineState->packages.cPackages; ++i)
            {
                BURN_PACKAGE* pPackage = pEngineState->packages.rgPackages + i;
                if (pPackage->fUninstallable)
                {
                    DetectPackageAsAbsent(pPackage);
                }
                else
                {
                    DetectPackageAsPresentAndCached(pPackage);
                    DetectPackageDependent(pPackage, pEngineState->registration.sczId);
                }
            }
        }

        BURN_RELATED_BUNDLE* DetectUpgradeBundle(
            __in BURN_ENGINE_STATE* pEngineState,
            __in LPCWSTR wzId,
            __in LPCWSTR wzVersion
            )
        {
            HRESULT hr = S_OK;
            BURN_RELATED_BUNDLES* pRelatedBundles = &pEngineState->registration.relatedBundles;
            BURN_DEPENDENCY_PROVIDER dependencyProvider = { };

            hr = StrAllocString(&dependencyProvider.sczKey, wzId, 0);
            NativeAssert::Succeeded(hr, "Failed to copy provider key");

            dependencyProvider.fImported = TRUE;

            hr = StrAllocString(&dependencyProvider.sczVersion, wzVersion, 0);
            NativeAssert::Succeeded(hr, "Failed to copy version");

            hr = MemEnsureArraySize(reinterpret_cast<LPVOID*>(&pRelatedBundles->rgRelatedBundles), pRelatedBundles->cRelatedBundles + 1, sizeof(BURN_RELATED_BUNDLE), 5);
            NativeAssert::Succeeded(hr, "Failed to ensure there is space for related bundles.");

            BURN_RELATED_BUNDLE* pRelatedBundle = pRelatedBundles->rgRelatedBundles + pRelatedBundles->cRelatedBundles;

            hr = VerParseVersion(wzVersion, 0, FALSE, &pRelatedBundle->pVersion);
            NativeAssert::Succeeded(hr, "Failed to parse pseudo bundle version: %ls", wzVersion);

            pRelatedBundle->fPlannable = TRUE;
            pRelatedBundle->relationType = BOOTSTRAPPER_RELATION_UPGRADE;

            hr = PseudoBundleInitialize(0, &pRelatedBundle->package, TRUE, wzId, pRelatedBundle->relationType, BOOTSTRAPPER_PACKAGE_STATE_PRESENT, TRUE, NULL, NULL, NULL, 0, FALSE, L"-quiet", L"-repair -quiet", L"-uninstall -quiet", &dependencyProvider, NULL, 0);
            NativeAssert::Succeeded(hr, "Failed to initialize related bundle to represent bundle: %ls", wzId);

            ++pRelatedBundles->cRelatedBundles;

            return pRelatedBundle;
        }

        void DetectAsRelatedUpgradeBundle(
            __in BURN_ENGINE_STATE* pEngineState,
            __in LPCWSTR wzId,
            __in LPCWSTR wzVersion
            )
        {
            HRESULT hr = StrAllocString(&pEngineState->registration.sczAncestors, wzId, 0);
            NativeAssert::Succeeded(hr, "Failed to set registration's ancestors");

            pEngineState->command.relationType = BOOTSTRAPPER_RELATION_UPGRADE;

            DetectPackagesAsPresentAndCached(pEngineState);
            DetectUpgradeBundle(pEngineState, wzId, wzVersion);

            for (DWORD i = 0; i < pEngineState->packages.cPackages; ++i)
            {
                BURN_PACKAGE* pPackage = pEngineState->packages.rgPackages + i;
                DetectPackageDependent(pPackage, wzId);
            }
        }

        void ValidateCacheContainer(
            __in BURN_PLAN* pPlan,
            __in BOOL fRollback,
            __in DWORD dwIndex,
            __in LPCWSTR wzContainerId
            )
        {
            BURN_CACHE_ACTION* pAction = ValidateCacheActionExists(pPlan, fRollback, dwIndex);
            Assert::Equal<DWORD>(BURN_CACHE_ACTION_TYPE_CONTAINER, pAction->type);
            NativeAssert::StringEqual(wzContainerId, pAction->container.pContainer->sczId);
        }

        BURN_CACHE_ACTION* ValidateCacheActionExists(BURN_PLAN* pPlan, BOOL fRollback, DWORD dwIndex)
        {
            Assert::InRange(dwIndex + 1ul, 1ul, (fRollback ? pPlan->cRollbackCacheActions : pPlan->cCacheActions));
            return (fRollback ? pPlan->rgRollbackCacheActions : pPlan->rgCacheActions) + dwIndex;
        }

        void ValidateCacheCheckpoint(
            __in BURN_PLAN* pPlan,
            __in BOOL fRollback,
            __in DWORD dwIndex,
            __in DWORD dwId
            )
        {
            BURN_CACHE_ACTION* pAction = ValidateCacheActionExists(pPlan, fRollback, dwIndex);
            Assert::Equal<DWORD>(BURN_CACHE_ACTION_TYPE_CHECKPOINT, pAction->type);
            Assert::Equal(dwId, pAction->checkpoint.dwId);
        }

        DWORD ValidateCachePackage(
            __in BURN_PLAN* pPlan,
            __in BOOL fRollback,
            __in DWORD dwIndex,
            __in LPCWSTR wzPackageId
            )
        {
            BURN_CACHE_ACTION* pAction = ValidateCacheActionExists(pPlan, fRollback, dwIndex);
            Assert::Equal<DWORD>(BURN_CACHE_ACTION_TYPE_PACKAGE, pAction->type);
            NativeAssert::StringEqual(wzPackageId, pAction->package.pPackage->sczId);
            return dwIndex + 1;
        }

        void ValidateCacheRollbackPackage(
            __in BURN_PLAN* pPlan,
            __in BOOL fRollback,
            __in DWORD dwIndex,
            __in LPCWSTR wzPackageId
            )
        {
            BURN_CACHE_ACTION* pAction = ValidateCacheActionExists(pPlan, fRollback, dwIndex);
            Assert::Equal<DWORD>(BURN_CACHE_ACTION_TYPE_ROLLBACK_PACKAGE, pAction->type);
            NativeAssert::StringEqual(wzPackageId, pAction->rollbackPackage.pPackage->sczId);
        }

        void ValidateCacheSignalSyncpoint(
            __in BURN_PLAN* pPlan,
            __in BOOL fRollback,
            __in DWORD dwIndex
            )
        {
            BURN_CACHE_ACTION* pAction = ValidateCacheActionExists(pPlan, fRollback, dwIndex);
            Assert::Equal<DWORD>(BURN_CACHE_ACTION_TYPE_SIGNAL_SYNCPOINT, pAction->type);
            Assert::NotEqual((DWORD_PTR)NULL, (DWORD_PTR)pAction->syncpoint.hEvent);
        }

        void ValidateCleanAction(
            __in BURN_PLAN* pPlan,
            __in DWORD dwIndex,
            __in LPCWSTR wzPackageId
            )
        {
            Assert::InRange(dwIndex + 1ul, 1ul, pPlan->cCleanActions);

            BURN_CLEAN_ACTION* pCleanAction = pPlan->rgCleanActions + dwIndex;
            Assert::NotEqual((DWORD_PTR)0, (DWORD_PTR)pCleanAction->pPackage);
            NativeAssert::StringEqual(wzPackageId, pCleanAction->pPackage->sczId);
        }

        BURN_EXECUTE_ACTION* ValidateExecuteActionExists(BURN_PLAN* pPlan, BOOL fRollback, DWORD dwIndex)
        {
            Assert::InRange(dwIndex + 1ul, 1ul, (fRollback ? pPlan->cRollbackActions : pPlan->cExecuteActions));
            return (fRollback ? pPlan->rgRollbackActions : pPlan->rgExecuteActions) + dwIndex;
        }

        void ValidateExecuteBeginMsiTransaction(
            __in BURN_PLAN* pPlan,
            __in BOOL fRollback,
            __in DWORD dwIndex,
            __in LPCWSTR wzRollbackBoundaryId
            )
        {
            BURN_EXECUTE_ACTION* pAction = ValidateExecuteActionExists(pPlan, fRollback, dwIndex);
            Assert::Equal<DWORD>(BURN_EXECUTE_ACTION_TYPE_BEGIN_MSI_TRANSACTION, pAction->type);
            NativeAssert::StringEqual(wzRollbackBoundaryId, pAction->msiTransaction.pRollbackBoundary->sczId);
            Assert::Equal<BOOL>(FALSE, pAction->fDeleted);
        }

        void ValidateExecuteCheckpoint(
            __in BURN_PLAN* pPlan,
            __in BOOL fRollback,
            __in DWORD dwIndex,
            __in DWORD dwId
            )
        {
            BURN_EXECUTE_ACTION* pAction = ValidateExecuteActionExists(pPlan, fRollback, dwIndex);
            Assert::Equal<DWORD>(BURN_EXECUTE_ACTION_TYPE_CHECKPOINT, pAction->type);
            Assert::Equal(dwId, pAction->checkpoint.dwId);
            Assert::Equal<BOOL>(FALSE, pAction->fDeleted);
        }

        void ValidateExecuteCommitMsiTransaction(
            __in BURN_PLAN* pPlan,
            __in BOOL fRollback,
            __in DWORD dwIndex,
            __in LPCWSTR wzRollbackBoundaryId
            )
        {
            BURN_EXECUTE_ACTION* pAction = ValidateExecuteActionExists(pPlan, fRollback, dwIndex);
            Assert::Equal<DWORD>(BURN_EXECUTE_ACTION_TYPE_COMMIT_MSI_TRANSACTION, pAction->type);
            NativeAssert::StringEqual(wzRollbackBoundaryId, pAction->msiTransaction.pRollbackBoundary->sczId);
            Assert::Equal<BOOL>(FALSE, pAction->fDeleted);
        }

        void ValidateExecuteExePackage(
            __in BURN_PLAN* pPlan,
            __in BOOL fRollback,
            __in DWORD dwIndex,
            __in LPCWSTR wzPackageId,
            __in BOOTSTRAPPER_ACTION_STATE action,
            __in LPCWSTR wzIgnoreDependencies
            )
        {
            BURN_EXECUTE_ACTION* pAction = ValidateExecuteActionExists(pPlan, fRollback, dwIndex);
            Assert::Equal<DWORD>(BURN_EXECUTE_ACTION_TYPE_EXE_PACKAGE, pAction->type);
            NativeAssert::StringEqual(wzPackageId, pAction->exePackage.pPackage->sczId);
            Assert::Equal<DWORD>(action, pAction->exePackage.action);
            NativeAssert::StringEqual(wzIgnoreDependencies, pAction->exePackage.sczIgnoreDependencies);
            Assert::Equal<BOOL>(FALSE, pAction->fDeleted);
        }

        void ValidateExecuteMsiPackage(
            __in BURN_PLAN* pPlan,
            __in BOOL fRollback,
            __in DWORD dwIndex,
            __in_z LPCWSTR wzPackageId,
            __in BOOTSTRAPPER_ACTION_STATE action,
            __in BURN_MSI_PROPERTY actionMsiProperty,
            __in DWORD uiLevel,
            __in BOOL fDisableExternalUiHandler,
            __in DWORD dwLoggingAttributes
            )
        {
            BURN_EXECUTE_ACTION* pAction = ValidateExecuteActionExists(pPlan, fRollback, dwIndex);
            Assert::Equal<DWORD>(BURN_EXECUTE_ACTION_TYPE_MSI_PACKAGE, pAction->type);
            NativeAssert::StringEqual(wzPackageId, pAction->msiPackage.pPackage->sczId);
            Assert::Equal<DWORD>(action, pAction->msiPackage.action);
            Assert::Equal<DWORD>(actionMsiProperty, pAction->msiPackage.actionMsiProperty);
            Assert::Equal<DWORD>(uiLevel, pAction->msiPackage.uiLevel);
            Assert::Equal<BOOL>(fDisableExternalUiHandler, pAction->msiPackage.fDisableExternalUiHandler);
            NativeAssert::NotNull(pAction->msiPackage.sczLogPath);
            Assert::Equal<DWORD>(dwLoggingAttributes, pAction->msiPackage.dwLoggingAttributes);
            Assert::Equal<BOOL>(FALSE, pAction->fDeleted);
        }

        BURN_EXECUTE_ACTION* ValidateDeletedExecuteMspTarget(
            __in BURN_PLAN* pPlan,
            __in BOOL fRollback,
            __in DWORD dwIndex,
            __in_z LPCWSTR wzPackageId,
            __in BOOTSTRAPPER_ACTION_STATE action,
            __in_z LPCWSTR wzTargetProductCode,
            __in BOOL fPerMachineTarget,
            __in BURN_MSI_PROPERTY actionMsiProperty,
            __in DWORD uiLevel,
            __in BOOL fDisableExternalUiHandler,
            __in BOOL fDeleted
            )
        {
            BURN_EXECUTE_ACTION* pAction = ValidateExecuteActionExists(pPlan, fRollback, dwIndex);
            Assert::Equal<DWORD>(BURN_EXECUTE_ACTION_TYPE_MSP_TARGET, pAction->type);
            NativeAssert::StringEqual(wzPackageId, pAction->mspTarget.pPackage->sczId);
            Assert::Equal<DWORD>(action, pAction->mspTarget.action);
            NativeAssert::StringEqual(wzTargetProductCode, pAction->mspTarget.sczTargetProductCode);
            Assert::Equal<BOOL>(fPerMachineTarget, pAction->mspTarget.fPerMachineTarget);
            Assert::Equal<DWORD>(actionMsiProperty, pAction->mspTarget.actionMsiProperty);
            Assert::Equal<DWORD>(uiLevel, pAction->mspTarget.uiLevel);
            Assert::Equal<BOOL>(fDisableExternalUiHandler, pAction->mspTarget.fDisableExternalUiHandler);
            NativeAssert::NotNull(pAction->mspTarget.sczLogPath);
            Assert::Equal<BOOL>(fDeleted, pAction->fDeleted);
            return pAction;
        }

        void ValidateExecuteMspTargetPatch(
            __in BURN_EXECUTE_ACTION* pAction,
            __in DWORD dwIndex,
            __in_z LPCWSTR wzPackageId
            )
        {
            Assert::InRange(dwIndex + 1ul, 1ul, pAction->mspTarget.cOrderedPatches);
            BURN_ORDERED_PATCHES* pOrderedPatch = pAction->mspTarget.rgOrderedPatches + dwIndex;
            NativeAssert::StringEqual(wzPackageId, pOrderedPatch->pPackage->sczId);
        }

        void ValidateExecutePackageDependency(
            __in BURN_PLAN* pPlan,
            __in BOOL fRollback,
            __in DWORD dwIndex,
            __in LPCWSTR wzPackageId,
            __in LPCWSTR wzBundleProviderKey,
            __in BURN_DEPENDENCY_ACTION action
            )
        {
            BURN_EXECUTE_ACTION* pAction = ValidateExecuteActionExists(pPlan, fRollback, dwIndex);
            Assert::Equal<DWORD>(BURN_EXECUTE_ACTION_TYPE_PACKAGE_DEPENDENCY, pAction->type);
            NativeAssert::StringEqual(wzPackageId, pAction->packageDependency.pPackage->sczId);
            NativeAssert::StringEqual(wzBundleProviderKey, pAction->packageDependency.sczBundleProviderKey);
            Assert::Equal<DWORD>(action, pAction->packageDependency.action);
            Assert::Equal<BOOL>(FALSE, pAction->fDeleted);
        }

        void ValidateExecutePackageProvider(
            __in BURN_PLAN* pPlan,
            __in BOOL fRollback,
            __in DWORD dwIndex,
            __in LPCWSTR wzPackageId,
            __in BURN_DEPENDENCY_ACTION action
            )
        {
            BURN_EXECUTE_ACTION* pAction = ValidateExecuteActionExists(pPlan, fRollback, dwIndex);
            Assert::Equal<DWORD>(BURN_EXECUTE_ACTION_TYPE_PACKAGE_PROVIDER, pAction->type);
            NativeAssert::StringEqual(wzPackageId, pAction->packageProvider.pPackage->sczId);
            Assert::Equal<DWORD>(action, pAction->packageProvider.action);
            Assert::Equal<BOOL>(FALSE, pAction->fDeleted);
        }

        void ValidateExecuteRollbackBoundary(
            __in BURN_PLAN* pPlan,
            __in BOOL fRollback,
            __in DWORD dwIndex,
            __in LPCWSTR wzId,
            __in BOOL fVital,
            __in BOOL fTransaction
            )
        {
            BURN_EXECUTE_ACTION* pAction = ValidateExecuteActionExists(pPlan, fRollback, dwIndex);
            Assert::Equal<DWORD>(BURN_EXECUTE_ACTION_TYPE_ROLLBACK_BOUNDARY, pAction->type);
            NativeAssert::StringEqual(wzId, pAction->rollbackBoundary.pRollbackBoundary->sczId);
            Assert::Equal<BOOL>(fVital, pAction->rollbackBoundary.pRollbackBoundary->fVital);
            Assert::Equal<BOOL>(fTransaction, pAction->rollbackBoundary.pRollbackBoundary->fTransaction);
            Assert::Equal<BOOL>(FALSE, pAction->fDeleted);
        }

        void ValidateExecuteUncachePackage(
            __in BURN_PLAN* pPlan,
            __in BOOL fRollback,
            __in DWORD dwIndex,
            __in LPCWSTR wzPackageId
            )
        {
            BURN_EXECUTE_ACTION* pAction = ValidateExecuteActionExists(pPlan, fRollback, dwIndex);
            Assert::Equal<DWORD>(BURN_EXECUTE_ACTION_TYPE_UNCACHE_PACKAGE, pAction->type);
            NativeAssert::StringEqual(wzPackageId, pAction->uncachePackage.pPackage->sczId);
            Assert::Equal<BOOL>(FALSE, pAction->fDeleted);
        }

        void ValidateExecuteWaitSyncpoint(
            __in BURN_PLAN* pPlan,
            __in BOOL fRollback,
            __in DWORD dwIndex,
            __in HANDLE hEvent
            )
        {
            BURN_EXECUTE_ACTION* pAction = ValidateExecuteActionExists(pPlan, fRollback, dwIndex);
            Assert::Equal<DWORD>(BURN_EXECUTE_ACTION_TYPE_WAIT_SYNCPOINT, pAction->type);
            Assert::Equal((DWORD_PTR)hEvent, (DWORD_PTR)pAction->syncpoint.hEvent);
            Assert::Equal<BOOL>(FALSE, pAction->fDeleted);
        }

        void ValidateNonPermanentPackageExpectedStates(
            __in BURN_PACKAGE* pPackage,
            __in_z LPCWSTR wzPackageId,
            __in BURN_PACKAGE_REGISTRATION_STATE expectedCacheState,
            __in BURN_PACKAGE_REGISTRATION_STATE expectedInstallState
            )
        {
            NativeAssert::StringEqual(wzPackageId, pPackage->sczId);
            Assert::Equal<BOOL>(TRUE, pPackage->fCanAffectRegistration);
            Assert::Equal<DWORD>(expectedCacheState, pPackage->expectedCacheRegistrationState);
            Assert::Equal<DWORD>(expectedInstallState, pPackage->expectedInstallRegistrationState);
        }

        void ValidatePermanentPackageExpectedStates(
            __in BURN_PACKAGE* pPackage,
            __in_z LPCWSTR wzPackageId
            )
        {
            NativeAssert::StringEqual(wzPackageId, pPackage->sczId);
            Assert::Equal<BOOL>(FALSE, pPackage->fCanAffectRegistration);
            Assert::Equal<DWORD>(BURN_PACKAGE_REGISTRATION_STATE_UNKNOWN, pPackage->expectedCacheRegistrationState);
            Assert::Equal<DWORD>(BURN_PACKAGE_REGISTRATION_STATE_UNKNOWN, pPackage->expectedInstallRegistrationState);
        }

        void ValidatePlannedProvider(
            __in BURN_PLAN* pPlan,
            __in UINT uIndex,
            __in LPCWSTR wzKey,
            __in LPCWSTR wzName
            )
        {
            Assert::InRange(uIndex + 1u, 1u, pPlan->cPlannedProviders);

            DEPENDENCY* pProvider = pPlan->rgPlannedProviders + uIndex;
            NativeAssert::StringEqual(wzKey, pProvider->sczKey);
            NativeAssert::StringEqual(wzName, pProvider->sczName);
        }
    };
}
}
}
}
}

static HRESULT WINAPI PlanTestBAProc(
    __in BOOTSTRAPPER_APPLICATION_MESSAGE /*message*/,
    __in const LPVOID /*pvArgs*/,
    __inout LPVOID /*pvResults*/,
    __in_opt LPVOID /*pvContext*/
    )
{
    return S_OK;
}
