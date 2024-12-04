#pragma once

#include <mlib/config.h>

#include <stdbool.h>

#if mlib_is_cxx()
#include <string>
#include <system_error>
#endif

typedef struct amongoc_status amongoc_status;

mlib_extern_c_begin();

/**
 * @brief Virtual table for customizing the behavior of `amongoc_status`
 *
 */
struct amongoc_status_category_vtable {
    // Get the name of the category
    const char* (*name)(void);
    // Dynamically allocate a new string for the message contained in the status
    char* (*strdup_message)(int code);
    // Test whether a particular integer value is an error
    bool (*is_error)(int code);
    // Test whether a particular integer value represents cancellation
    bool (*is_cancellation)(int code);
    // Test whether a particular integer value represents timeout
    bool (*is_timeout)(int code);
};

// An X-list of status category names, invoked as: `X(Identifier, C++ class)
#define AMONGOC_STATUS_CATEGORY_X_LIST()                                                           \
    X(generic)                                                                                     \
    X(system)                                                                                      \
    X(netdb)                                                                                       \
    X(addrinfo)                                                                                    \
    X(io)                                                                                          \
    X(server)                                                                                      \
    X(client)                                                                                      \
    X(tls)                                                                                         \
    X(unknown)

#define X(Ident)                                                                                   \
    /* C category global instance */                                                               \
    extern const struct amongoc_status_category_vtable MLIB_PASTE_3(amongoc_, Ident, _category);   \
    /* C++ category getter */                                                                      \
    MLIB_IF_CXX(namespace amongoc {                                                                \
        extern "C++" std::error_category const& MLIB_PASTE(Ident, _category)() noexcept;           \
    })
AMONGOC_STATUS_CATEGORY_X_LIST()
#undef X

enum amongoc_io_errc {
    amongoc_errc_connection_closed = 1,
    amongoc_errc_short_read        = 2,
};

enum amongoc_client_errc {
    amongoc_client_errc_okay = 0,
    /**
     * @brief Indicates that the update document given for an update operation is
     * invalid.
     */
    amongoc_client_errc_invalid_update_document = 1,
};

enum amongoc_server_errc {
    amongoc_server_errc_InternalError                                               = 1,
    amongoc_server_errc_BadValue                                                    = 2,
    amongoc_server_errc_NoSuchKey                                                   = 4,
    amongoc_server_errc_GraphContainsCycle                                          = 5,
    amongoc_server_errc_HostUnreachable                                             = 6,
    amongoc_server_errc_HostNotFound                                                = 7,
    amongoc_server_errc_UnknownError                                                = 8,
    amongoc_server_errc_FailedToParse                                               = 9,
    amongoc_server_errc_CannotMutateObject                                          = 10,
    amongoc_server_errc_UserNotFound                                                = 11,
    amongoc_server_errc_UnsupportedFormat                                           = 12,
    amongoc_server_errc_Unauthorized                                                = 13,
    amongoc_server_errc_TypeMismatch                                                = 14,
    amongoc_server_errc_Overflow                                                    = 15,
    amongoc_server_errc_InvalidLength                                               = 16,
    amongoc_server_errc_ProtocolError                                               = 17,
    amongoc_server_errc_AuthenticationFailed                                        = 18,
    amongoc_server_errc_CannotReuseObject                                           = 19,
    amongoc_server_errc_IllegalOperation                                            = 20,
    amongoc_server_errc_EmptyArrayOperation                                         = 21,
    amongoc_server_errc_InvalidBSON                                                 = 22,
    amongoc_server_errc_AlreadyInitialized                                          = 23,
    amongoc_server_errc_LockTimeout                                                 = 24,
    amongoc_server_errc_RemoteValidationError                                       = 25,
    amongoc_server_errc_NamespaceNotFound                                           = 26,
    amongoc_server_errc_IndexNotFound                                               = 27,
    amongoc_server_errc_PathNotViable                                               = 28,
    amongoc_server_errc_NonExistentPath                                             = 29,
    amongoc_server_errc_InvalidPath                                                 = 30,
    amongoc_server_errc_RoleNotFound                                                = 31,
    amongoc_server_errc_RolesNotRelated                                             = 32,
    amongoc_server_errc_PrivilegeNotFound                                           = 33,
    amongoc_server_errc_CannotBackfillArray                                         = 34,
    amongoc_server_errc_UserModificationFailed                                      = 35,
    amongoc_server_errc_RemoteChangeDetected                                        = 36,
    amongoc_server_errc_FileRenameFailed                                            = 37,
    amongoc_server_errc_FileNotOpen                                                 = 38,
    amongoc_server_errc_FileStreamFailed                                            = 39,
    amongoc_server_errc_ConflictingUpdateOperators                                  = 40,
    amongoc_server_errc_FileAlreadyOpen                                             = 41,
    amongoc_server_errc_LogWriteFailed                                              = 42,
    amongoc_server_errc_CursorNotFound                                              = 43,
    amongoc_server_errc_UserDataInconsistent                                        = 45,
    amongoc_server_errc_LockBusy                                                    = 46,
    amongoc_server_errc_NoMatchingDocument                                          = 47,
    amongoc_server_errc_NamespaceExists                                             = 48,
    amongoc_server_errc_InvalidRoleModification                                     = 49,
    amongoc_server_errc_MaxTimeMSExpired                                            = 50,
    amongoc_server_errc_ManualInterventionRequired                                  = 51,
    amongoc_server_errc_DollarPrefixedFieldName                                     = 52,
    amongoc_server_errc_InvalidIdField                                              = 53,
    amongoc_server_errc_NotSingleValueField                                         = 54,
    amongoc_server_errc_InvalidDBRef                                                = 55,
    amongoc_server_errc_EmptyFieldName                                              = 56,
    amongoc_server_errc_DottedFieldName                                             = 57,
    amongoc_server_errc_RoleModificationFailed                                      = 58,
    amongoc_server_errc_CommandNotFound                                             = 59,
    amongoc_server_errc_ShardKeyNotFound                                            = 61,
    amongoc_server_errc_OplogOperationUnsupported                                   = 62,
    amongoc_server_errc_StaleShardVersion                                           = 63,
    amongoc_server_errc_WriteConcernFailed                                          = 64,
    amongoc_server_errc_MultipleErrorsOccurred                                      = 65,
    amongoc_server_errc_ImmutableField                                              = 66,
    amongoc_server_errc_CannotCreateIndex                                           = 67,
    amongoc_server_errc_IndexAlreadyExists                                          = 68,
    amongoc_server_errc_AuthSchemaIncompatible                                      = 69,
    amongoc_server_errc_ShardNotFound                                               = 70,
    amongoc_server_errc_ReplicaSetNotFound                                          = 71,
    amongoc_server_errc_InvalidOptions                                              = 72,
    amongoc_server_errc_InvalidNamespace                                            = 73,
    amongoc_server_errc_NodeNotFound                                                = 74,
    amongoc_server_errc_WriteConcernLegacyOK                                        = 75,
    amongoc_server_errc_NoReplicationEnabled                                        = 76,
    amongoc_server_errc_OperationIncomplete                                         = 77,
    amongoc_server_errc_CommandResultSchemaViolation                                = 78,
    amongoc_server_errc_UnknownReplWriteConcern                                     = 79,
    amongoc_server_errc_RoleDataInconsistent                                        = 80,
    amongoc_server_errc_NoMatchParseContext                                         = 81,
    amongoc_server_errc_NoProgressMade                                              = 82,
    amongoc_server_errc_RemoteResultsUnavailable                                    = 83,
    amongoc_server_errc_IndexOptionsConflict                                        = 85,
    amongoc_server_errc_IndexKeySpecsConflict                                       = 86,
    amongoc_server_errc_CannotSplit                                                 = 87,
    amongoc_server_errc_NetworkTimeout                                              = 89,
    amongoc_server_errc_CallbackCanceled                                            = 90,
    amongoc_server_errc_ShutdownInProgress                                          = 91,
    amongoc_server_errc_SecondaryAheadOfPrimary                                     = 92,
    amongoc_server_errc_InvalidReplicaSetConfig                                     = 93,
    amongoc_server_errc_NotYetInitialized                                           = 94,
    amongoc_server_errc_NotSecondary                                                = 95,
    amongoc_server_errc_OperationFailed                                             = 96,
    amongoc_server_errc_NoProjectionFound                                           = 97,
    amongoc_server_errc_DBPathInUse                                                 = 98,
    amongoc_server_errc_UnsatisfiableWriteConcern                                   = 100,
    amongoc_server_errc_OutdatedClient                                              = 101,
    amongoc_server_errc_IncompatibleAuditMetadata                                   = 102,
    amongoc_server_errc_NewReplicaSetConfigurationIncompatible                      = 103,
    amongoc_server_errc_NodeNotElectable                                            = 104,
    amongoc_server_errc_IncompatibleShardingMetadata                                = 105,
    amongoc_server_errc_DistributedClockSkewed                                      = 106,
    amongoc_server_errc_LockFailed                                                  = 107,
    amongoc_server_errc_InconsistentReplicaSetNames                                 = 108,
    amongoc_server_errc_ConfigurationInProgress                                     = 109,
    amongoc_server_errc_CannotInitializeNodeWithData                                = 110,
    amongoc_server_errc_NotExactValueField                                          = 111,
    amongoc_server_errc_WriteConflict                                               = 112,
    amongoc_server_errc_InitialSyncFailure                                          = 113,
    amongoc_server_errc_InitialSyncOplogSourceMissing                               = 114,
    amongoc_server_errc_CommandNotSupported                                         = 115,
    amongoc_server_errc_DocTooLargeForCapped                                        = 116,
    amongoc_server_errc_ConflictingOperationInProgress                              = 117,
    amongoc_server_errc_NamespaceNotSharded                                         = 118,
    amongoc_server_errc_InvalidSyncSource                                           = 119,
    amongoc_server_errc_OplogStartMissing                                           = 120,
    amongoc_server_errc_DocumentValidationFailure                                   = 121,
    amongoc_server_errc_NotAReplicaSet                                              = 123,
    amongoc_server_errc_IncompatibleElectionProtocol                                = 124,
    amongoc_server_errc_CommandFailed                                               = 125,
    amongoc_server_errc_RPCProtocolNegotiationFailed                                = 126,
    amongoc_server_errc_UnrecoverableRollbackError                                  = 127,
    amongoc_server_errc_LockNotFound                                                = 128,
    amongoc_server_errc_LockStateChangeFailed                                       = 129,
    amongoc_server_errc_SymbolNotFound                                              = 130,
    amongoc_server_errc_FailedToSatisfyReadPreference                               = 133,
    amongoc_server_errc_ReadConcernMajorityNotAvailableYet                          = 134,
    amongoc_server_errc_StaleTerm                                                   = 135,
    amongoc_server_errc_CappedPositionLost                                          = 136,
    amongoc_server_errc_IncompatibleShardingConfigVersion                           = 137,
    amongoc_server_errc_RemoteOplogStale                                            = 138,
    amongoc_server_errc_JSInterpreterFailure                                        = 139,
    amongoc_server_errc_InvalidSSLConfiguration                                     = 140,
    amongoc_server_errc_SSLHandshakeFailed                                          = 141,
    amongoc_server_errc_JSUncatchableError                                          = 142,
    amongoc_server_errc_CursorInUse                                                 = 143,
    amongoc_server_errc_IncompatibleCatalogManager                                  = 144,
    amongoc_server_errc_PooledConnectionsDropped                                    = 145,
    amongoc_server_errc_ExceededMemoryLimit                                         = 146,
    amongoc_server_errc_ZLibError                                                   = 147,
    amongoc_server_errc_ReadConcernMajorityNotEnabled                               = 148,
    amongoc_server_errc_NoConfigPrimary                                             = 149,
    amongoc_server_errc_StaleEpoch                                                  = 150,
    amongoc_server_errc_OperationCannotBeBatched                                    = 151,
    amongoc_server_errc_OplogOutOfOrder                                             = 152,
    amongoc_server_errc_ChunkTooBig                                                 = 153,
    amongoc_server_errc_InconsistentShardIdentity                                   = 154,
    amongoc_server_errc_CannotApplyOplogWhilePrimary                                = 155,
    amongoc_server_errc_CanRepairToDowngrade                                        = 157,
    amongoc_server_errc_MustUpgrade                                                 = 158,
    amongoc_server_errc_DurationOverflow                                            = 159,
    amongoc_server_errc_MaxStalenessOutOfRange                                      = 160,
    amongoc_server_errc_IncompatibleCollationVersion                                = 161,
    amongoc_server_errc_CollectionIsEmpty                                           = 162,
    amongoc_server_errc_ZoneStillInUse                                              = 163,
    amongoc_server_errc_InitialSyncActive                                           = 164,
    amongoc_server_errc_ViewDepthLimitExceeded                                      = 165,
    amongoc_server_errc_CommandNotSupportedOnView                                   = 166,
    amongoc_server_errc_OptionNotSupportedOnView                                    = 167,
    amongoc_server_errc_InvalidPipelineOperator                                     = 168,
    amongoc_server_errc_CommandOnShardedViewNotSupportedOnMongod                    = 169,
    amongoc_server_errc_TooManyMatchingDocuments                                    = 170,
    amongoc_server_errc_CannotIndexParallelArrays                                   = 171,
    amongoc_server_errc_TransportSessionClosed                                      = 172,
    amongoc_server_errc_TransportSessionNotFound                                    = 173,
    amongoc_server_errc_TransportSessionUnknown                                     = 174,
    amongoc_server_errc_QueryPlanKilled                                             = 175,
    amongoc_server_errc_FileOpenFailed                                              = 176,
    amongoc_server_errc_ZoneNotFound                                                = 177,
    amongoc_server_errc_RangeOverlapConflict                                        = 178,
    amongoc_server_errc_WindowsPdhError                                             = 179,
    amongoc_server_errc_BadPerfCounterPath                                          = 180,
    amongoc_server_errc_AmbiguousIndexKeyPattern                                    = 181,
    amongoc_server_errc_InvalidViewDefinition                                       = 182,
    amongoc_server_errc_ClientMetadataMissingField                                  = 183,
    amongoc_server_errc_ClientMetadataAppNameTooLarge                               = 184,
    amongoc_server_errc_ClientMetadataDocumentTooLarge                              = 185,
    amongoc_server_errc_ClientMetadataCannotBeMutated                               = 186,
    amongoc_server_errc_LinearizableReadConcernError                                = 187,
    amongoc_server_errc_IncompatibleServerVersion                                   = 188,
    amongoc_server_errc_PrimarySteppedDown                                          = 189,
    amongoc_server_errc_MasterSlaveConnectionFailure                                = 190,
    amongoc_server_errc_FailPointEnabled                                            = 192,
    amongoc_server_errc_NoShardingEnabled                                           = 193,
    amongoc_server_errc_BalancerInterrupted                                         = 194,
    amongoc_server_errc_ViewPipelineMaxSizeExceeded                                 = 195,
    amongoc_server_errc_InvalidIndexSpecificationOption                             = 197,
    amongoc_server_errc_ReplicaSetMonitorRemoved                                    = 199,
    amongoc_server_errc_ChunkRangeCleanupPending                                    = 200,
    amongoc_server_errc_CannotBuildIndexKeys                                        = 201,
    amongoc_server_errc_NetworkInterfaceExceededTimeLimit                           = 202,
    amongoc_server_errc_ShardingStateNotInitialized                                 = 203,
    amongoc_server_errc_TimeProofMismatch                                           = 204,
    amongoc_server_errc_ClusterTimeFailsRateLimiter                                 = 205,
    amongoc_server_errc_NoSuchSession                                               = 206,
    amongoc_server_errc_InvalidUUID                                                 = 207,
    amongoc_server_errc_TooManyLocks                                                = 208,
    amongoc_server_errc_StaleClusterTime                                            = 209,
    amongoc_server_errc_CannotVerifyAndSignLogicalTime                              = 210,
    amongoc_server_errc_KeyNotFound                                                 = 211,
    amongoc_server_errc_IncompatibleRollbackAlgorithm                               = 212,
    amongoc_server_errc_DuplicateSession                                            = 213,
    amongoc_server_errc_AuthenticationRestrictionUnmet                              = 214,
    amongoc_server_errc_DatabaseDropPending                                         = 215,
    amongoc_server_errc_ElectionInProgress                                          = 216,
    amongoc_server_errc_IncompleteTransactionHistory                                = 217,
    amongoc_server_errc_UpdateOperationFailed                                       = 218,
    amongoc_server_errc_FTDCPathNotSet                                              = 219,
    amongoc_server_errc_FTDCPathAlreadySet                                          = 220,
    amongoc_server_errc_IndexModified                                               = 221,
    amongoc_server_errc_CloseChangeStream                                           = 222,
    amongoc_server_errc_IllegalOpMsgFlag                                            = 223,
    amongoc_server_errc_QueryFeatureNotAllowed                                      = 224,
    amongoc_server_errc_TransactionTooOld                                           = 225,
    amongoc_server_errc_AtomicityFailure                                            = 226,
    amongoc_server_errc_CannotImplicitlyCreateCollection                            = 227,
    amongoc_server_errc_SessionTransferIncomplete                                   = 228,
    amongoc_server_errc_MustDowngrade                                               = 229,
    amongoc_server_errc_DNSHostNotFound                                             = 230,
    amongoc_server_errc_DNSProtocolError                                            = 231,
    amongoc_server_errc_MaxSubPipelineDepthExceeded                                 = 232,
    amongoc_server_errc_TooManyDocumentSequences                                    = 233,
    amongoc_server_errc_RetryChangeStream                                           = 234,
    amongoc_server_errc_InternalErrorNotSupported                                   = 235,
    amongoc_server_errc_ForTestingErrorExtraInfo                                    = 236,
    amongoc_server_errc_CursorKilled                                                = 237,
    amongoc_server_errc_NotImplemented                                              = 238,
    amongoc_server_errc_SnapshotTooOld                                              = 239,
    amongoc_server_errc_DNSRecordTypeMismatch                                       = 240,
    amongoc_server_errc_ConversionFailure                                           = 241,
    amongoc_server_errc_CannotCreateCollection                                      = 242,
    amongoc_server_errc_IncompatibleWithUpgradedServer                              = 243,
    amongoc_server_errc_BrokenPromise                                               = 245,
    amongoc_server_errc_SnapshotUnavailable                                         = 246,
    amongoc_server_errc_ProducerConsumerQueueBatchTooLarge                          = 247,
    amongoc_server_errc_ProducerConsumerQueueEndClosed                              = 248,
    amongoc_server_errc_StaleDbVersion                                              = 249,
    amongoc_server_errc_StaleChunkHistory                                           = 250,
    amongoc_server_errc_NoSuchTransaction                                           = 251,
    amongoc_server_errc_ReentrancyNotAllowed                                        = 252,
    amongoc_server_errc_FreeMonHttpInFlight                                         = 253,
    amongoc_server_errc_FreeMonHttpTemporaryFailure                                 = 254,
    amongoc_server_errc_FreeMonHttpPermanentFailure                                 = 255,
    amongoc_server_errc_TransactionCommitted                                        = 256,
    amongoc_server_errc_TransactionTooLarge                                         = 257,
    amongoc_server_errc_UnknownFeatureCompatibilityVersion                          = 258,
    amongoc_server_errc_KeyedExecutorRetry                                          = 259,
    amongoc_server_errc_InvalidResumeToken                                          = 260,
    amongoc_server_errc_TooManyLogicalSessions                                      = 261,
    amongoc_server_errc_ExceededTimeLimit                                           = 262,
    amongoc_server_errc_OperationNotSupportedInTransaction                          = 263,
    amongoc_server_errc_TooManyFilesOpen                                            = 264,
    amongoc_server_errc_OrphanedRangeCleanUpFailed                                  = 265,
    amongoc_server_errc_FailPointSetFailed                                          = 266,
    amongoc_server_errc_PreparedTransactionInProgress                               = 267,
    amongoc_server_errc_CannotBackup                                                = 268,
    amongoc_server_errc_DataModifiedByRepair                                        = 269,
    amongoc_server_errc_RepairedReplicaSetNode                                      = 270,
    amongoc_server_errc_JSInterpreterFailureWithStack                               = 271,
    amongoc_server_errc_MigrationConflict                                           = 272,
    amongoc_server_errc_ProducerConsumerQueueProducerQueueDepthExceeded             = 273,
    amongoc_server_errc_ProducerConsumerQueueConsumed                               = 274,
    amongoc_server_errc_ExchangePassthrough                                         = 275,
    amongoc_server_errc_IndexBuildAborted                                           = 276,
    amongoc_server_errc_AlarmAlreadyFulfilled                                       = 277,
    amongoc_server_errc_UnsatisfiableCommitQuorum                                   = 278,
    amongoc_server_errc_ClientDisconnect                                            = 279,
    amongoc_server_errc_ChangeStreamFatalError                                      = 280,
    amongoc_server_errc_TransactionCoordinatorSteppingDown                          = 281,
    amongoc_server_errc_TransactionCoordinatorReachedAbortDecision                  = 282,
    amongoc_server_errc_WouldChangeOwningShard                                      = 283,
    amongoc_server_errc_ForTestingErrorExtraInfoWithExtraInfoInNamespace            = 284,
    amongoc_server_errc_IndexBuildAlreadyInProgress                                 = 285,
    amongoc_server_errc_ChangeStreamHistoryLost                                     = 286,
    amongoc_server_errc_TransactionCoordinatorDeadlineTaskCanceled                  = 287,
    amongoc_server_errc_ChecksumMismatch                                            = 288,
    amongoc_server_errc_WaitForMajorityServiceEarlierOpTimeAvailable                = 289,
    amongoc_server_errc_TransactionExceededLifetimeLimitSeconds                     = 290,
    amongoc_server_errc_NoQueryExecutionPlans                                       = 291,
    amongoc_server_errc_QueryExceededMemoryLimitNoDiskUseAllowed                    = 292,
    amongoc_server_errc_InvalidSeedList                                             = 293,
    amongoc_server_errc_InvalidTopologyType                                         = 294,
    amongoc_server_errc_InvalidHeartBeatFrequency                                   = 295,
    amongoc_server_errc_TopologySetNameRequired                                     = 296,
    amongoc_server_errc_HierarchicalAcquisitionLevelViolation                       = 297,
    amongoc_server_errc_InvalidServerType                                           = 298,
    amongoc_server_errc_OCSPCertificateStatusRevoked                                = 299,
    amongoc_server_errc_RangeDeletionAbandonedBecauseCollectionWithUUIDDoesNotExist = 300,
    amongoc_server_errc_DataCorruptionDetected                                      = 301,
    amongoc_server_errc_OCSPCertificateStatusUnknown                                = 302,
    amongoc_server_errc_SplitHorizonChange                                          = 303,
    amongoc_server_errc_ShardInvalidatedForTargeting                                = 304,
    amongoc_server_errc_RangeDeletionAbandonedBecauseTaskDocumentDoesNotExist       = 307,
    amongoc_server_errc_CurrentConfigNotCommittedYet                                = 308,
    amongoc_server_errc_ExhaustCommandFinished                                      = 309,
    amongoc_server_errc_PeriodicJobIsStopped                                        = 310,
    amongoc_server_errc_TransactionCoordinatorCanceled                              = 311,
    amongoc_server_errc_OperationIsKilledAndDelisted                                = 312,
    amongoc_server_errc_ResumableRangeDeleterDisabled                               = 313,
    amongoc_server_errc_ObjectIsBusy                                                = 314,
    amongoc_server_errc_TooStaleToSyncFromSource                                    = 315,
    amongoc_server_errc_QueryTrialRunCompleted                                      = 316,
    amongoc_server_errc_ConnectionPoolExpired                                       = 317,
    amongoc_server_errc_ForTestingOptionalErrorExtraInfo                            = 318,
    amongoc_server_errc_MovePrimaryInProgress                                       = 319,
    amongoc_server_errc_TenantMigrationConflict                                     = 320,
    amongoc_server_errc_TenantMigrationCommitted                                    = 321,
    amongoc_server_errc_APIVersionError                                             = 322,
    amongoc_server_errc_APIStrictError                                              = 323,
    amongoc_server_errc_APIDeprecationError                                         = 324,
    amongoc_server_errc_TenantMigrationAborted                                      = 325,
    amongoc_server_errc_OplogQueryMinTsMissing                                      = 326,
    amongoc_server_errc_NoSuchTenantMigration                                       = 327,
    amongoc_server_errc_TenantMigrationAccessBlockerShuttingDown                    = 328,
    amongoc_server_errc_TenantMigrationInProgress                                   = 329,
    amongoc_server_errc_SkipCommandExecution                                        = 330,
    amongoc_server_errc_FailedToRunWithReplyBuilder                                 = 331,
    amongoc_server_errc_CannotDowngrade                                             = 332,
    amongoc_server_errc_ServiceExecutorInShutdown                                   = 333,
    amongoc_server_errc_MechanismUnavailable                                        = 334,
    amongoc_server_errc_TenantMigrationForgotten                                    = 335,
    amongoc_server_errc_SocketException                                             = 9001,
    amongoc_server_errc_CannotGrowDocumentInCappedNamespace                         = 10003,
    amongoc_server_errc_NotWritablePrimary                                          = 10107,
    amongoc_server_errc_BSONObjectTooLarge                                          = 10334,
    amongoc_server_errc_DuplicateKey                                                = 11000,
    amongoc_server_errc_InterruptedAtShutdown                                       = 11600,
    amongoc_server_errc_Interrupted                                                 = 11601,
    amongoc_server_errc_InterruptedDueToReplStateChange                             = 11602,
    amongoc_server_errc_BackgroundOperationInProgressForDatabase                    = 12586,
    amongoc_server_errc_BackgroundOperationInProgressForNamespace                   = 12587,
    amongoc_server_errc_MergeStageNoMatchingDocument                                = 13113,
    amongoc_server_errc_DatabaseDifferCase                                          = 13297,
    amongoc_server_errc_StaleConfig                                                 = 13388,
    amongoc_server_errc_NotPrimaryNoSecondaryOk                                     = 13435,
    amongoc_server_errc_NotPrimaryOrSecondary                                       = 13436,
    amongoc_server_errc_OutOfDiskSpace                                              = 14031,
    amongoc_server_errc_ClientMarkedKilled                                          = 46841,
};

/**
 * @brief Error codes related to TLS.
 *
 * The `ossl_` codes correspond to the reason codes in `sslerr.h` from OpenSSL
 */
enum amongoc_tls_errc {
    amongoc_tls_errc_okay                                                = 0,
    amongoc_tls_errc_ossl_application_data_after_close_notify            = 291,
    amongoc_tls_errc_ossl_app_data_in_handshake                          = 100,
    amongoc_tls_errc_ossl_attempt_to_reuse_session_in_different_context  = 272,
    amongoc_tls_errc_ossl_at_least_tls_1_2_needed_in_suiteb_mode         = 158,
    amongoc_tls_errc_ossl_bad_certificate                                = 348,
    amongoc_tls_errc_ossl_bad_change_cipher_spec                         = 103,
    amongoc_tls_errc_ossl_bad_cipher                                     = 186,
    amongoc_tls_errc_ossl_bad_compression_algorithm                      = 326,
    amongoc_tls_errc_ossl_bad_data                                       = 390,
    amongoc_tls_errc_ossl_bad_data_returned_by_callback                  = 106,
    amongoc_tls_errc_ossl_bad_decompression                              = 107,
    amongoc_tls_errc_ossl_bad_dh_value                                   = 102,
    amongoc_tls_errc_ossl_bad_digest_length                              = 111,
    amongoc_tls_errc_ossl_bad_early_data                                 = 233,
    amongoc_tls_errc_ossl_bad_ecc_cert                                   = 304,
    amongoc_tls_errc_ossl_bad_ecpoint                                    = 306,
    amongoc_tls_errc_ossl_bad_extension                                  = 110,
    amongoc_tls_errc_ossl_bad_handshake_length                           = 332,
    amongoc_tls_errc_ossl_bad_handshake_state                            = 236,
    amongoc_tls_errc_ossl_bad_hello_request                              = 105,
    amongoc_tls_errc_ossl_bad_hrr_version                                = 263,
    amongoc_tls_errc_ossl_bad_key_share                                  = 108,
    amongoc_tls_errc_ossl_bad_key_update                                 = 122,
    amongoc_tls_errc_ossl_bad_legacy_version                             = 292,
    amongoc_tls_errc_ossl_bad_length                                     = 271,
    amongoc_tls_errc_ossl_bad_packet                                     = 240,
    amongoc_tls_errc_ossl_bad_packet_length                              = 115,
    amongoc_tls_errc_ossl_bad_protocol_version_number                    = 116,
    amongoc_tls_errc_ossl_bad_psk                                        = 219,
    amongoc_tls_errc_ossl_bad_psk_identity                               = 114,
    amongoc_tls_errc_ossl_bad_record_type                                = 443,
    amongoc_tls_errc_ossl_bad_rsa_encrypt                                = 119,
    amongoc_tls_errc_ossl_bad_signature                                  = 123,
    amongoc_tls_errc_ossl_bad_srp_a_length                               = 347,
    amongoc_tls_errc_ossl_bad_srp_parameters                             = 371,
    amongoc_tls_errc_ossl_bad_srtp_mki_value                             = 352,
    amongoc_tls_errc_ossl_bad_srtp_protection_profile_list               = 353,
    amongoc_tls_errc_ossl_bad_ssl_filetype                               = 124,
    amongoc_tls_errc_ossl_bad_value                                      = 384,
    amongoc_tls_errc_ossl_bad_write_retry                                = 127,
    amongoc_tls_errc_ossl_binder_does_not_verify                         = 253,
    amongoc_tls_errc_ossl_bio_not_set                                    = 128,
    amongoc_tls_errc_ossl_block_cipher_pad_is_wrong                      = 129,
    amongoc_tls_errc_ossl_bn_lib                                         = 130,
    amongoc_tls_errc_ossl_callback_failed                                = 234,
    amongoc_tls_errc_ossl_cannot_change_cipher                           = 109,
    amongoc_tls_errc_ossl_cannot_get_group_name                          = 299,
    amongoc_tls_errc_ossl_ca_dn_length_mismatch                          = 131,
    amongoc_tls_errc_ossl_ca_key_too_small                               = 397,
    amongoc_tls_errc_ossl_ca_md_too_weak                                 = 398,
    amongoc_tls_errc_ossl_ccs_received_early                             = 133,
    amongoc_tls_errc_ossl_certificate_verify_failed                      = 134,
    amongoc_tls_errc_ossl_cert_cb_error                                  = 377,
    amongoc_tls_errc_ossl_cert_length_mismatch                           = 135,
    amongoc_tls_errc_ossl_ciphersuite_digest_has_changed                 = 218,
    amongoc_tls_errc_ossl_cipher_code_wrong_length                       = 137,
    amongoc_tls_errc_ossl_clienthello_tlsext                             = 226,
    amongoc_tls_errc_ossl_compressed_length_too_long                     = 140,
    amongoc_tls_errc_ossl_compression_disabled                           = 343,
    amongoc_tls_errc_ossl_compression_failure                            = 141,
    amongoc_tls_errc_ossl_compression_id_not_within_private_range        = 307,
    amongoc_tls_errc_ossl_compression_library_error                      = 142,
    amongoc_tls_errc_ossl_connection_type_not_set                        = 144,
    amongoc_tls_errc_ossl_conn_use_only                                  = 356,
    amongoc_tls_errc_ossl_context_not_dane_enabled                       = 167,
    amongoc_tls_errc_ossl_cookie_gen_callback_failure                    = 400,
    amongoc_tls_errc_ossl_cookie_mismatch                                = 308,
    amongoc_tls_errc_ossl_copy_parameters_failed                         = 296,
    amongoc_tls_errc_ossl_custom_ext_handler_already_installed           = 206,
    amongoc_tls_errc_ossl_dane_already_enabled                           = 172,
    amongoc_tls_errc_ossl_dane_cannot_override_mtype_full                = 173,
    amongoc_tls_errc_ossl_dane_not_enabled                               = 175,
    amongoc_tls_errc_ossl_dane_tlsa_bad_certificate                      = 180,
    amongoc_tls_errc_ossl_dane_tlsa_bad_certificate_usage                = 184,
    amongoc_tls_errc_ossl_dane_tlsa_bad_data_length                      = 189,
    amongoc_tls_errc_ossl_dane_tlsa_bad_digest_length                    = 192,
    amongoc_tls_errc_ossl_dane_tlsa_bad_matching_type                    = 200,
    amongoc_tls_errc_ossl_dane_tlsa_bad_public_key                       = 201,
    amongoc_tls_errc_ossl_dane_tlsa_bad_selector                         = 202,
    amongoc_tls_errc_ossl_dane_tlsa_null_data                            = 203,
    amongoc_tls_errc_ossl_data_between_ccs_and_finished                  = 145,
    amongoc_tls_errc_ossl_data_length_too_long                           = 146,
    amongoc_tls_errc_ossl_decryption_failed                              = 147,
    amongoc_tls_errc_ossl_decryption_failed_or_bad_record_mac            = 281,
    amongoc_tls_errc_ossl_dh_key_too_small                               = 394,
    amongoc_tls_errc_ossl_dh_public_value_length_is_wrong                = 148,
    amongoc_tls_errc_ossl_digest_check_failed                            = 149,
    amongoc_tls_errc_ossl_dtls_message_too_big                           = 334,
    amongoc_tls_errc_ossl_duplicate_compression_id                       = 309,
    amongoc_tls_errc_ossl_ecc_cert_not_for_signing                       = 318,
    amongoc_tls_errc_ossl_ecdh_required_for_suiteb_mode                  = 374,
    amongoc_tls_errc_ossl_ee_key_too_small                               = 399,
    amongoc_tls_errc_ossl_empty_raw_public_key                           = 349,
    amongoc_tls_errc_ossl_empty_srtp_protection_profile_list             = 354,
    amongoc_tls_errc_ossl_encrypted_length_too_long                      = 150,
    amongoc_tls_errc_ossl_error_in_received_cipher_list                  = 151,
    amongoc_tls_errc_ossl_error_setting_tlsa_base_domain                 = 204,
    amongoc_tls_errc_ossl_exceeds_max_fragment_size                      = 194,
    amongoc_tls_errc_ossl_excessive_message_size                         = 152,
    amongoc_tls_errc_ossl_extension_not_received                         = 279,
    amongoc_tls_errc_ossl_extra_data_in_message                          = 153,
    amongoc_tls_errc_ossl_ext_length_mismatch                            = 163,
    amongoc_tls_errc_ossl_failed_to_get_parameter                        = 316,
    amongoc_tls_errc_ossl_failed_to_init_async                           = 405,
    amongoc_tls_errc_ossl_feature_negotiation_not_complete               = 417,
    amongoc_tls_errc_ossl_feature_not_renegotiable                       = 413,
    amongoc_tls_errc_ossl_fragmented_client_hello                        = 401,
    amongoc_tls_errc_ossl_got_a_fin_before_a_ccs                         = 154,
    amongoc_tls_errc_ossl_https_proxy_request                            = 155,
    amongoc_tls_errc_ossl_http_request                                   = 156,
    amongoc_tls_errc_ossl_illegal_point_compression                      = 162,
    amongoc_tls_errc_ossl_illegal_suiteb_digest                          = 380,
    amongoc_tls_errc_ossl_inappropriate_fallback                         = 373,
    amongoc_tls_errc_ossl_inconsistent_compression                       = 340,
    amongoc_tls_errc_ossl_inconsistent_early_data_alpn                   = 222,
    amongoc_tls_errc_ossl_inconsistent_early_data_sni                    = 231,
    amongoc_tls_errc_ossl_inconsistent_extms                             = 104,
    amongoc_tls_errc_ossl_insufficient_security                          = 241,
    amongoc_tls_errc_ossl_invalid_alert                                  = 205,
    amongoc_tls_errc_ossl_invalid_ccs_message                            = 260,
    amongoc_tls_errc_ossl_invalid_certificate_or_alg                     = 238,
    amongoc_tls_errc_ossl_invalid_command                                = 280,
    amongoc_tls_errc_ossl_invalid_compression_algorithm                  = 341,
    amongoc_tls_errc_ossl_invalid_config                                 = 283,
    amongoc_tls_errc_ossl_invalid_configuration_name                     = 113,
    amongoc_tls_errc_ossl_invalid_context                                = 282,
    amongoc_tls_errc_ossl_invalid_ct_validation_type                     = 212,
    amongoc_tls_errc_ossl_invalid_key_update_type                        = 120,
    amongoc_tls_errc_ossl_invalid_max_early_data                         = 174,
    amongoc_tls_errc_ossl_invalid_null_cmd_name                          = 385,
    amongoc_tls_errc_ossl_invalid_raw_public_key                         = 350,
    amongoc_tls_errc_ossl_invalid_record                                 = 317,
    amongoc_tls_errc_ossl_invalid_sequence_number                        = 402,
    amongoc_tls_errc_ossl_invalid_serverinfo_data                        = 388,
    amongoc_tls_errc_ossl_invalid_session_id                             = 999,
    amongoc_tls_errc_ossl_invalid_srp_username                           = 357,
    amongoc_tls_errc_ossl_invalid_status_response                        = 328,
    amongoc_tls_errc_ossl_invalid_ticket_keys_length                     = 325,
    amongoc_tls_errc_ossl_legacy_sigalg_disallowed_or_unsupported        = 333,
    amongoc_tls_errc_ossl_length_mismatch                                = 159,
    amongoc_tls_errc_ossl_length_too_long                                = 404,
    amongoc_tls_errc_ossl_length_too_short                               = 160,
    amongoc_tls_errc_ossl_library_bug                                    = 274,
    amongoc_tls_errc_ossl_library_has_no_ciphers                         = 161,
    amongoc_tls_errc_ossl_maximum_encrypted_pkts_reached                 = 395,
    amongoc_tls_errc_ossl_missing_dsa_signing_cert                       = 165,
    amongoc_tls_errc_ossl_missing_ecdsa_signing_cert                     = 381,
    amongoc_tls_errc_ossl_missing_fatal                                  = 256,
    amongoc_tls_errc_ossl_missing_parameters                             = 290,
    amongoc_tls_errc_ossl_missing_psk_kex_modes_extension                = 310,
    amongoc_tls_errc_ossl_missing_rsa_certificate                        = 168,
    amongoc_tls_errc_ossl_missing_rsa_encrypting_cert                    = 169,
    amongoc_tls_errc_ossl_missing_rsa_signing_cert                       = 170,
    amongoc_tls_errc_ossl_missing_sigalgs_extension                      = 112,
    amongoc_tls_errc_ossl_missing_signing_cert                           = 221,
    amongoc_tls_errc_ossl_missing_srp_param                              = 358,
    amongoc_tls_errc_ossl_missing_supported_groups_extension             = 209,
    amongoc_tls_errc_ossl_missing_tmp_dh_key                             = 171,
    amongoc_tls_errc_ossl_missing_tmp_ecdh_key                           = 311,
    amongoc_tls_errc_ossl_mixed_handshake_and_non_handshake_data         = 293,
    amongoc_tls_errc_ossl_not_on_record_boundary                         = 182,
    amongoc_tls_errc_ossl_not_replacing_certificate                      = 289,
    amongoc_tls_errc_ossl_not_server                                     = 284,
    amongoc_tls_errc_ossl_no_application_protocol                        = 235,
    amongoc_tls_errc_ossl_no_certificates_returned                       = 176,
    amongoc_tls_errc_ossl_no_certificate_assigned                        = 177,
    amongoc_tls_errc_ossl_no_certificate_set                             = 179,
    amongoc_tls_errc_ossl_no_change_following_hrr                        = 214,
    amongoc_tls_errc_ossl_no_ciphers_available                           = 181,
    amongoc_tls_errc_ossl_no_ciphers_specified                           = 183,
    amongoc_tls_errc_ossl_no_cipher_match                                = 185,
    amongoc_tls_errc_ossl_no_client_cert_method                          = 331,
    amongoc_tls_errc_ossl_no_compression_specified                       = 187,
    amongoc_tls_errc_ossl_no_cookie_callback_set                         = 287,
    amongoc_tls_errc_ossl_no_gost_certificate_sent_by_peer               = 330,
    amongoc_tls_errc_ossl_no_method_specified                            = 188,
    amongoc_tls_errc_ossl_no_pem_extensions                              = 389,
    amongoc_tls_errc_ossl_no_private_key_assigned                        = 190,
    amongoc_tls_errc_ossl_no_protocols_available                         = 191,
    amongoc_tls_errc_ossl_no_renegotiation                               = 339,
    amongoc_tls_errc_ossl_no_required_digest                             = 324,
    amongoc_tls_errc_ossl_no_shared_cipher                               = 193,
    amongoc_tls_errc_ossl_no_shared_groups                               = 410,
    amongoc_tls_errc_ossl_no_shared_signature_algorithms                 = 376,
    amongoc_tls_errc_ossl_no_srtp_profiles                               = 359,
    amongoc_tls_errc_ossl_no_stream                                      = 355,
    amongoc_tls_errc_ossl_no_suitable_digest_algorithm                   = 297,
    amongoc_tls_errc_ossl_no_suitable_groups                             = 295,
    amongoc_tls_errc_ossl_no_suitable_key_share                          = 101,
    amongoc_tls_errc_ossl_no_suitable_record_layer                       = 322,
    amongoc_tls_errc_ossl_no_suitable_signature_algorithm                = 118,
    amongoc_tls_errc_ossl_no_valid_scts                                  = 216,
    amongoc_tls_errc_ossl_no_verify_cookie_callback                      = 403,
    amongoc_tls_errc_ossl_null_ssl_ctx                                   = 195,
    amongoc_tls_errc_ossl_null_ssl_method_passed                         = 196,
    amongoc_tls_errc_ossl_ocsp_callback_failure                          = 305,
    amongoc_tls_errc_ossl_old_session_cipher_not_returned                = 197,
    amongoc_tls_errc_ossl_old_session_compression_algorithm_not_returned = 344,
    amongoc_tls_errc_ossl_overflow_error                                 = 237,
    amongoc_tls_errc_ossl_packet_length_too_long                         = 198,
    amongoc_tls_errc_ossl_parse_tlsext                                   = 227,
    amongoc_tls_errc_ossl_path_too_long                                  = 270,
    amongoc_tls_errc_ossl_peer_did_not_return_a_certificate              = 199,
    amongoc_tls_errc_ossl_pem_name_bad_prefix                            = 391,
    amongoc_tls_errc_ossl_pem_name_too_short                             = 392,
    amongoc_tls_errc_ossl_pipeline_failure                               = 406,
    amongoc_tls_errc_ossl_poll_request_not_supported                     = 418,
    amongoc_tls_errc_ossl_post_handshake_auth_encoding_err               = 278,
    amongoc_tls_errc_ossl_private_key_mismatch                           = 288,
    amongoc_tls_errc_ossl_protocol_is_shutdown                           = 207,
    amongoc_tls_errc_ossl_psk_identity_not_found                         = 223,
    amongoc_tls_errc_ossl_psk_no_client_cb                               = 224,
    amongoc_tls_errc_ossl_psk_no_server_cb                               = 225,
    amongoc_tls_errc_ossl_quic_handshake_layer_error                     = 393,
    amongoc_tls_errc_ossl_quic_network_error                             = 387,
    amongoc_tls_errc_ossl_quic_protocol_error                            = 382,
    amongoc_tls_errc_ossl_read_bio_not_set                               = 211,
    amongoc_tls_errc_ossl_read_timeout_expired                           = 312,
    amongoc_tls_errc_ossl_records_not_released                           = 321,
    amongoc_tls_errc_ossl_record_layer_failure                           = 313,
    amongoc_tls_errc_ossl_record_length_mismatch                         = 213,
    amongoc_tls_errc_ossl_record_too_small                               = 298,
    amongoc_tls_errc_ossl_remote_peer_address_not_set                    = 346,
    amongoc_tls_errc_ossl_renegotiate_ext_too_long                       = 335,
    amongoc_tls_errc_ossl_renegotiation_encoding_err                     = 336,
    amongoc_tls_errc_ossl_renegotiation_mismatch                         = 337,
    amongoc_tls_errc_ossl_request_pending                                = 285,
    amongoc_tls_errc_ossl_request_sent                                   = 286,
    amongoc_tls_errc_ossl_required_cipher_missing                        = 215,
    amongoc_tls_errc_ossl_required_compression_algorithm_missing         = 342,
    amongoc_tls_errc_ossl_scsv_received_when_renegotiating               = 345,
    amongoc_tls_errc_ossl_sct_verification_failed                        = 208,
    amongoc_tls_errc_ossl_sequence_ctr_wrapped                           = 327,
    amongoc_tls_errc_ossl_serverhello_tlsext                             = 275,
    amongoc_tls_errc_ossl_session_id_context_uninitialized               = 277,
    amongoc_tls_errc_ossl_shutdown_while_in_init                         = 407,
    amongoc_tls_errc_ossl_signature_algorithms_error                     = 360,
    amongoc_tls_errc_ossl_signature_for_non_signing_certificate          = 220,
    amongoc_tls_errc_ossl_srp_a_calc                                     = 361,
    amongoc_tls_errc_ossl_srtp_could_not_allocate_profiles               = 362,
    amongoc_tls_errc_ossl_srtp_protection_profile_list_too_long          = 363,
    amongoc_tls_errc_ossl_srtp_unknown_protection_profile                = 364,
    amongoc_tls_errc_ossl_ssl3_ext_invalid_max_fragment_length           = 232,
    amongoc_tls_errc_ossl_ssl3_ext_invalid_servername                    = 319,
    amongoc_tls_errc_ossl_ssl3_ext_invalid_servername_type               = 320,
    amongoc_tls_errc_ossl_ssl3_session_id_too_long                       = 300,
    amongoc_tls_errc_ossl_sslv3_alert_bad_certificate                    = 1042,
    amongoc_tls_errc_ossl_sslv3_alert_bad_record_mac                     = 1020,
    amongoc_tls_errc_ossl_sslv3_alert_certificate_expired                = 1045,
    amongoc_tls_errc_ossl_sslv3_alert_certificate_revoked                = 1044,
    amongoc_tls_errc_ossl_sslv3_alert_certificate_unknown                = 1046,
    amongoc_tls_errc_ossl_sslv3_alert_decompression_failure              = 1030,
    amongoc_tls_errc_ossl_sslv3_alert_handshake_failure                  = 1040,
    amongoc_tls_errc_ossl_sslv3_alert_illegal_parameter                  = 1047,
    amongoc_tls_errc_ossl_sslv3_alert_no_certificate                     = 1041,
    amongoc_tls_errc_ossl_sslv3_alert_unexpected_message                 = 1010,
    amongoc_tls_errc_ossl_sslv3_alert_unsupported_certificate            = 1043,
    amongoc_tls_errc_ossl_ssl_command_section_empty                      = 117,
    amongoc_tls_errc_ossl_ssl_command_section_not_found                  = 125,
    amongoc_tls_errc_ossl_ssl_ctx_has_no_default_ssl_version             = 228,
    amongoc_tls_errc_ossl_ssl_handshake_failure                          = 229,
    amongoc_tls_errc_ossl_ssl_library_has_no_ciphers                     = 230,
    amongoc_tls_errc_ossl_ssl_negative_length                            = 372,
    amongoc_tls_errc_ossl_ssl_section_empty                              = 126,
    amongoc_tls_errc_ossl_ssl_section_not_found                          = 136,
    amongoc_tls_errc_ossl_ssl_session_id_callback_failed                 = 301,
    amongoc_tls_errc_ossl_ssl_session_id_conflict                        = 302,
    amongoc_tls_errc_ossl_ssl_session_id_context_too_long                = 273,
    amongoc_tls_errc_ossl_ssl_session_id_has_bad_length                  = 303,
    amongoc_tls_errc_ossl_ssl_session_id_too_long                        = 408,
    amongoc_tls_errc_ossl_ssl_session_version_mismatch                   = 210,
    amongoc_tls_errc_ossl_still_in_init                                  = 121,
    amongoc_tls_errc_ossl_stream_count_limited                           = 411,
    amongoc_tls_errc_ossl_stream_finished                                = 365,
    amongoc_tls_errc_ossl_stream_recv_only                               = 366,
    amongoc_tls_errc_ossl_stream_reset                                   = 375,
    amongoc_tls_errc_ossl_stream_send_only                               = 379,
    amongoc_tls_errc_ossl_tlsv13_alert_certificate_required              = 1116,
    amongoc_tls_errc_ossl_tlsv13_alert_missing_extension                 = 1109,
    amongoc_tls_errc_ossl_tlsv1_alert_access_denied                      = 1049,
    amongoc_tls_errc_ossl_tlsv1_alert_decode_error                       = 1050,
    amongoc_tls_errc_ossl_tlsv1_alert_decryption_failed                  = 1021,
    amongoc_tls_errc_ossl_tlsv1_alert_decrypt_error                      = 1051,
    amongoc_tls_errc_ossl_tlsv1_alert_export_restriction                 = 1060,
    amongoc_tls_errc_ossl_tlsv1_alert_inappropriate_fallback             = 1086,
    amongoc_tls_errc_ossl_tlsv1_alert_insufficient_security              = 1071,
    amongoc_tls_errc_ossl_tlsv1_alert_internal_error                     = 1080,
    amongoc_tls_errc_ossl_tlsv1_alert_no_application_protocol            = 1120,
    amongoc_tls_errc_ossl_tlsv1_alert_no_renegotiation                   = 1100,
    amongoc_tls_errc_ossl_tlsv1_alert_protocol_version                   = 1070,
    amongoc_tls_errc_ossl_tlsv1_alert_record_overflow                    = 1022,
    amongoc_tls_errc_ossl_tlsv1_alert_unknown_ca                         = 1048,
    amongoc_tls_errc_ossl_tlsv1_alert_unknown_psk_identity               = 1115,
    amongoc_tls_errc_ossl_tlsv1_alert_user_cancelled                     = 1090,
    amongoc_tls_errc_ossl_tlsv1_bad_certificate_hash_value               = 1114,
    amongoc_tls_errc_ossl_tlsv1_bad_certificate_status_response          = 1113,
    amongoc_tls_errc_ossl_tlsv1_certificate_unobtainable                 = 1111,
    amongoc_tls_errc_ossl_tlsv1_unrecognized_name                        = 1112,
    amongoc_tls_errc_ossl_tlsv1_unsupported_extension                    = 1110,
    amongoc_tls_errc_ossl_tls_illegal_exporter_label                     = 367,
    amongoc_tls_errc_ossl_tls_invalid_ecpointformat_list                 = 157,
    amongoc_tls_errc_ossl_too_many_key_updates                           = 132,
    amongoc_tls_errc_ossl_too_many_warn_alerts                           = 409,
    amongoc_tls_errc_ossl_too_much_early_data                            = 164,
    amongoc_tls_errc_ossl_unable_to_find_ecdh_parameters                 = 314,
    amongoc_tls_errc_ossl_unable_to_find_public_key_parameters           = 239,
    amongoc_tls_errc_ossl_unable_to_load_ssl3_md5_routines               = 242,
    amongoc_tls_errc_ossl_unable_to_load_ssl3_sha1_routines              = 243,
    amongoc_tls_errc_ossl_unexpected_ccs_message                         = 262,
    amongoc_tls_errc_ossl_unexpected_end_of_early_data                   = 178,
    amongoc_tls_errc_ossl_unexpected_eof_while_reading                   = 294,
    amongoc_tls_errc_ossl_unexpected_message                             = 244,
    amongoc_tls_errc_ossl_unexpected_record                              = 245,
    amongoc_tls_errc_ossl_uninitialized                                  = 276,
    amongoc_tls_errc_ossl_unknown_alert_type                             = 246,
    amongoc_tls_errc_ossl_unknown_certificate_type                       = 247,
    amongoc_tls_errc_ossl_unknown_cipher_returned                        = 248,
    amongoc_tls_errc_ossl_unknown_cipher_type                            = 249,
    amongoc_tls_errc_ossl_unknown_cmd_name                               = 386,
    amongoc_tls_errc_ossl_unknown_command                                = 139,
    amongoc_tls_errc_ossl_unknown_digest                                 = 368,
    amongoc_tls_errc_ossl_unknown_key_exchange_type                      = 250,
    amongoc_tls_errc_ossl_unknown_mandatory_parameter                    = 323,
    amongoc_tls_errc_ossl_unknown_pkey_type                              = 251,
    amongoc_tls_errc_ossl_unknown_protocol                               = 252,
    amongoc_tls_errc_ossl_unknown_ssl_version                            = 254,
    amongoc_tls_errc_ossl_unknown_state                                  = 255,
    amongoc_tls_errc_ossl_unsafe_legacy_renegotiation_disabled           = 338,
    amongoc_tls_errc_ossl_unsolicited_extension                          = 217,
    amongoc_tls_errc_ossl_unsupported_compression_algorithm              = 257,
    amongoc_tls_errc_ossl_unsupported_config_value                       = 414,
    amongoc_tls_errc_ossl_unsupported_config_value_class                 = 415,
    amongoc_tls_errc_ossl_unsupported_config_value_op                    = 416,
    amongoc_tls_errc_ossl_unsupported_elliptic_curve                     = 315,
    amongoc_tls_errc_ossl_unsupported_protocol                           = 258,
    amongoc_tls_errc_ossl_unsupported_ssl_version                        = 259,
    amongoc_tls_errc_ossl_unsupported_status_type                        = 329,
    amongoc_tls_errc_ossl_unsupported_write_flag                         = 412,
    amongoc_tls_errc_ossl_use_srtp_not_negotiated                        = 369,
    amongoc_tls_errc_ossl_version_too_high                               = 166,
    amongoc_tls_errc_ossl_version_too_low                                = 396,
    amongoc_tls_errc_ossl_wrong_certificate_type                         = 383,
    amongoc_tls_errc_ossl_wrong_cipher_returned                          = 261,
    amongoc_tls_errc_ossl_wrong_curve                                    = 378,
    amongoc_tls_errc_ossl_wrong_rpk_type                                 = 351,
    amongoc_tls_errc_ossl_wrong_signature_length                         = 264,
    amongoc_tls_errc_ossl_wrong_signature_size                           = 265,
    amongoc_tls_errc_ossl_wrong_signature_type                           = 370,
    amongoc_tls_errc_ossl_wrong_ssl_version                              = 266,
    amongoc_tls_errc_ossl_wrong_version_number                           = 267,
    amongoc_tls_errc_ossl_x509_lib                                       = 268,
    amongoc_tls_errc_ossl_x509_verification_setup_problems               = 269,
};

struct amongoc_status {
    struct amongoc_status_category_vtable const* category;
    // The error code integer value
    int code;

#if mlib_is_cxx()
    // Construct an amongoc_status from a std::error_code
    static amongoc_status from(std::error_code const&) noexcept;
    static amongoc_status from(std::errc ec) noexcept { return from(std::make_error_code(ec)); }

    /**
     * @brief Convert a status to a `std::error_code`
     *
     * @warning THIS IS LOSSY! The status may have a custom error category that
     * cannot be mapped automatically to a C++ error category. This method should
     * only be used on status values that have a category beloning to amongoc
     */
    std::error_code as_error_code() const noexcept;

    // Obtain the message string associated with this status
    std::string message() const noexcept;

    // Return `true` if the status represents an error
    inline bool is_error() const noexcept;

    constexpr bool operator==(amongoc_status const& other) const noexcept {
        return category == other.category and code == other.code;
    }
    constexpr bool operator!=(amongoc_status const& other) const noexcept {
        return not(*this == other);
    }
#endif
};

/**
 * @brief Test whether the given status code represents an error condition.
 */
static inline bool amongoc_is_error(amongoc_status st) mlib_noexcept {
    // If the category defines a way to check for errors, ask the category
    if (st.category->is_error) {
        return st.category->is_error(st.code);
    }
    // The category says nothing about what is an error, so consider any non-zero value to be an
    // error
    return st.code != 0;
}

/**
 * @brief Return `true` if the given status represents a cancellation
 */
static inline bool amongoc_is_cancellation(amongoc_status st) mlib_noexcept {
    return st.category->is_cancellation && st.category->is_cancellation(st.code);
}

/**
 * @brief Return `true` if the given status represents an operational time-out
 */
static inline bool amongoc_is_timeout(amongoc_status st) mlib_noexcept {
    return st.category->is_timeout && st.category->is_timeout(st.code);
}

/**
 * @brief Obtain a human-readable message describing the given status
 *
 * @return char* A dynamically allocated null-terminated C string describing the status.
 * @note The returned string must be freed with free()!
 */
static inline char* amongoc_status_strdup_message(amongoc_status s) {
    return s.category->strdup_message(s.code);
}

/**
 * @brief Obtain the reason code if-and-only-if the given status corresponds to a TLS error
 *
 * @param st A status code which may be a TLS error
 * @return amongoc_tls_errc Returns `amongoc_tls_errc_okay` (zero) if the status is not from TLS,
 * otherwise returns the non-zero reason code.
 */
enum amongoc_tls_errc amongoc_status_tls_reason(amongoc_status st) mlib_noexcept;

mlib_always_inline amongoc_status const* _amongocStatusGetOkayStatus(void) mlib_noexcept {
    static const amongoc_status ret = {&amongoc_generic_category, 0};
    return &ret;
}

mlib_extern_c_end();

#define amongoc_okay mlib_parenthesized_expression(*_amongocStatusGetOkayStatus())

#if mlib_is_cxx()

namespace amongoc {

using status = ::amongoc_status;

/**
 * @brief A basic exception type that carries an `amongoc_status` value
 */
class exception : std::runtime_error {
public:
    explicit exception(amongoc_status s) noexcept
        : runtime_error(s.message())
        , _status(s) {}

    // Get the status associated with this exception
    amongoc_status status() const noexcept { return _status; }

private:
    amongoc_status _status;
};

}  // namespace amongoc

bool amongoc_status::is_error() const noexcept { return amongoc_is_error(*this); }

#endif
