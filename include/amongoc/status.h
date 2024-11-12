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

mlib_extern_c_end();

extern const struct amongoc_status_category_vtable amongoc_generic_category;
extern const struct amongoc_status_category_vtable amongoc_system_category;
extern const struct amongoc_status_category_vtable amongoc_netdb_category;
extern const struct amongoc_status_category_vtable amongoc_addrinfo_category;
extern const struct amongoc_status_category_vtable amongoc_io_category;
extern const struct amongoc_status_category_vtable amongoc_server_category;
extern const struct amongoc_status_category_vtable amongoc_client_category;
extern const struct amongoc_status_category_vtable amongoc_unknown_category;

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

struct amongoc_status {
    struct amongoc_status_category_vtable const* category;
    // The error code integer value
    int code;

#if mlib_is_cxx()
    amongoc_status() noexcept
        : category(&amongoc_generic_category)
        , code(0) {}

    amongoc_status(struct amongoc_status_category_vtable const* cat, int code) noexcept
        : category(cat)
        , code(code) {}

    // Allow conversion from literal zero
    amongoc_status(decltype(nullptr)) noexcept
        : amongoc_status() {}

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

mlib_extern_c_begin();

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

const std::error_category& io_category() noexcept;
const std::error_category& server_category() noexcept;
const std::error_category& client_category() noexcept;

}  // namespace amongoc

bool amongoc_status::is_error() const noexcept { return amongoc_is_error(*this); }

#endif
