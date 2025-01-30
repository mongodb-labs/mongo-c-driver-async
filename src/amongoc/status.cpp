#include <amongoc/status.h>

#include <asio/error.hpp>
#include <fmt/format.h>
#include <openssl/err.h>

#include <cerrno>
#include <cstring>
#include <system_error>

using namespace amongoc;

namespace {

class unknown_error_category : public std::error_category {
    const char* name() const noexcept override { return "amongoc.unknown"; }
    std::string message(int ec) const noexcept override {
        return fmt::format("amongoc.unknown:{}", ec);
    }
} unknown_category_inst;

struct io_category_cls : std::error_category {
    const char* name() const noexcept override { return "amongoc.io"; }
    std::string message(int ec) const noexcept override {
        char buf[128];
        return message_noalloc(ec, buf, sizeof buf);
    }

    const char* message_noalloc(int ec, char* buf, size_t buflen) const noexcept {
        switch (static_cast<::amongoc_io_errc>(ec)) {
        case ::amongoc_errc_connection_closed:
            return "connection closed";
        case ::amongoc_errc_short_read:
            return "short read";
        default:
            std::snprintf(buf, buflen, "amongoc.io:%d", ec);
            return buf;
        }
    }
} io_category_inst;

struct server_category_cls : std::error_category {
    const char* name() const noexcept override { return "amongoc.server"; }
    std::string message(int ec) const noexcept override {
        char buf[128];
        return this->message_noalloc(ec, buf, sizeof buf);
    }

    const char* message_noalloc(int ec, char* buf, size_t buflen) const noexcept {
        auto str = _message_cstr(ec);
        if (str) {
            return str;
        }
        std::snprintf(buf, buflen, "amongoc.server:%d", ec);
        return buf;
    }

    const char* _message_cstr(int ec) const noexcept {
        switch (static_cast<amongoc_server_errc>(ec)) {
        case amongoc_server_errc_InternalError:
            return "InternalError";
        case amongoc_server_errc_BadValue:
            return "BadValue";
        case amongoc_server_errc_NoSuchKey:
            return "NoSuchKey";
        case amongoc_server_errc_GraphContainsCycle:
            return "GraphContainsCycle";
        case amongoc_server_errc_HostUnreachable:
            return "HostUnreachable";
        case amongoc_server_errc_HostNotFound:
            return "HostNotFound";
        case amongoc_server_errc_UnknownError:
            return "UnknownError";
        case amongoc_server_errc_FailedToParse:
            return "FailedToParse";
        case amongoc_server_errc_CannotMutateObject:
            return "CannotMutateObject";
        case amongoc_server_errc_UserNotFound:
            return "UserNotFound";
        case amongoc_server_errc_UnsupportedFormat:
            return "UnsupportedFormat";
        case amongoc_server_errc_Unauthorized:
            return "Unauthorized";
        case amongoc_server_errc_TypeMismatch:
            return "TypeMismatch";
        case amongoc_server_errc_Overflow:
            return "Overflow";
        case amongoc_server_errc_InvalidLength:
            return "InvalidLength";
        case amongoc_server_errc_ProtocolError:
            return "ProtocolError";
        case amongoc_server_errc_AuthenticationFailed:
            return "AuthenticationFailed";
        case amongoc_server_errc_CannotReuseObject:
            return "CannotReuseObject";
        case amongoc_server_errc_IllegalOperation:
            return "IllegalOperation";
        case amongoc_server_errc_EmptyArrayOperation:
            return "EmptyArrayOperation";
        case amongoc_server_errc_InvalidBSON:
            return "InvalidBSON";
        case amongoc_server_errc_AlreadyInitialized:
            return "AlreadyInitialized";
        case amongoc_server_errc_LockTimeout:
            return "LockTimeout";
        case amongoc_server_errc_RemoteValidationError:
            return "RemoteValidationError";
        case amongoc_server_errc_NamespaceNotFound:
            return "NamespaceNotFound";
        case amongoc_server_errc_IndexNotFound:
            return "IndexNotFound";
        case amongoc_server_errc_PathNotViable:
            return "PathNotViable";
        case amongoc_server_errc_NonExistentPath:
            return "NonExistentPath";
        case amongoc_server_errc_InvalidPath:
            return "InvalidPath";
        case amongoc_server_errc_RoleNotFound:
            return "RoleNotFound";
        case amongoc_server_errc_RolesNotRelated:
            return "RolesNotRelated";
        case amongoc_server_errc_PrivilegeNotFound:
            return "PrivilegeNotFound";
        case amongoc_server_errc_CannotBackfillArray:
            return "CannotBackfillArray";
        case amongoc_server_errc_UserModificationFailed:
            return "UserModificationFailed";
        case amongoc_server_errc_RemoteChangeDetected:
            return "RemoteChangeDetected";
        case amongoc_server_errc_FileRenameFailed:
            return "FileRenameFailed";
        case amongoc_server_errc_FileNotOpen:
            return "FileNotOpen";
        case amongoc_server_errc_FileStreamFailed:
            return "FileStreamFailed";
        case amongoc_server_errc_ConflictingUpdateOperators:
            return "ConflictingUpdateOperators";
        case amongoc_server_errc_FileAlreadyOpen:
            return "FileAlreadyOpen";
        case amongoc_server_errc_LogWriteFailed:
            return "LogWriteFailed";
        case amongoc_server_errc_CursorNotFound:
            return "CursorNotFound";
        case amongoc_server_errc_UserDataInconsistent:
            return "UserDataInconsistent";
        case amongoc_server_errc_LockBusy:
            return "LockBusy";
        case amongoc_server_errc_NoMatchingDocument:
            return "NoMatchingDocument";
        case amongoc_server_errc_NamespaceExists:
            return "NamespaceExists";
        case amongoc_server_errc_InvalidRoleModification:
            return "InvalidRoleModification";
        case amongoc_server_errc_MaxTimeMSExpired:
            return "MaxTimeMSExpired";
        case amongoc_server_errc_ManualInterventionRequired:
            return "ManualInterventionRequired";
        case amongoc_server_errc_DollarPrefixedFieldName:
            return "DollarPrefixedFieldName";
        case amongoc_server_errc_InvalidIdField:
            return "InvalidIdField";
        case amongoc_server_errc_NotSingleValueField:
            return "NotSingleValueField";
        case amongoc_server_errc_InvalidDBRef:
            return "InvalidDBRef";
        case amongoc_server_errc_EmptyFieldName:
            return "EmptyFieldName";
        case amongoc_server_errc_DottedFieldName:
            return "DottedFieldName";
        case amongoc_server_errc_RoleModificationFailed:
            return "RoleModificationFailed";
        case amongoc_server_errc_CommandNotFound:
            return "CommandNotFound";
        case amongoc_server_errc_ShardKeyNotFound:
            return "ShardKeyNotFound";
        case amongoc_server_errc_OplogOperationUnsupported:
            return "OplogOperationUnsupported";
        case amongoc_server_errc_StaleShardVersion:
            return "StaleShardVersion";
        case amongoc_server_errc_WriteConcernFailed:
            return "WriteConcernFailed";
        case amongoc_server_errc_MultipleErrorsOccurred:
            return "MultipleErrorsOccurred";
        case amongoc_server_errc_ImmutableField:
            return "ImmutableField";
        case amongoc_server_errc_CannotCreateIndex:
            return "CannotCreateIndex";
        case amongoc_server_errc_IndexAlreadyExists:
            return "IndexAlreadyExists";
        case amongoc_server_errc_AuthSchemaIncompatible:
            return "AuthSchemaIncompatible";
        case amongoc_server_errc_ShardNotFound:
            return "ShardNotFound";
        case amongoc_server_errc_ReplicaSetNotFound:
            return "ReplicaSetNotFound";
        case amongoc_server_errc_InvalidOptions:
            return "InvalidOptions";
        case amongoc_server_errc_InvalidNamespace:
            return "InvalidNamespace";
        case amongoc_server_errc_NodeNotFound:
            return "NodeNotFound";
        case amongoc_server_errc_WriteConcernLegacyOK:
            return "WriteConcernLegacyOK";
        case amongoc_server_errc_NoReplicationEnabled:
            return "NoReplicationEnabled";
        case amongoc_server_errc_OperationIncomplete:
            return "OperationIncomplete";
        case amongoc_server_errc_CommandResultSchemaViolation:
            return "CommandResultSchemaViolation";
        case amongoc_server_errc_UnknownReplWriteConcern:
            return "UnknownReplWriteConcern";
        case amongoc_server_errc_RoleDataInconsistent:
            return "RoleDataInconsistent";
        case amongoc_server_errc_NoMatchParseContext:
            return "NoMatchParseContext";
        case amongoc_server_errc_NoProgressMade:
            return "NoProgressMade";
        case amongoc_server_errc_RemoteResultsUnavailable:
            return "RemoteResultsUnavailable";
        case amongoc_server_errc_IndexOptionsConflict:
            return "IndexOptionsConflict";
        case amongoc_server_errc_IndexKeySpecsConflict:
            return "IndexKeySpecsConflict";
        case amongoc_server_errc_CannotSplit:
            return "CannotSplit";
        case amongoc_server_errc_NetworkTimeout:
            return "NetworkTimeout";
        case amongoc_server_errc_CallbackCanceled:
            return "CallbackCanceled";
        case amongoc_server_errc_ShutdownInProgress:
            return "ShutdownInProgress";
        case amongoc_server_errc_SecondaryAheadOfPrimary:
            return "SecondaryAheadOfPrimary";
        case amongoc_server_errc_InvalidReplicaSetConfig:
            return "InvalidReplicaSetConfig";
        case amongoc_server_errc_NotYetInitialized:
            return "NotYetInitialized";
        case amongoc_server_errc_NotSecondary:
            return "NotSecondary";
        case amongoc_server_errc_OperationFailed:
            return "OperationFailed";
        case amongoc_server_errc_NoProjectionFound:
            return "NoProjectionFound";
        case amongoc_server_errc_DBPathInUse:
            return "DBPathInUse";
        case amongoc_server_errc_UnsatisfiableWriteConcern:
            return "UnsatisfiableWriteConcern";
        case amongoc_server_errc_OutdatedClient:
            return "OutdatedClient";
        case amongoc_server_errc_IncompatibleAuditMetadata:
            return "IncompatibleAuditMetadata";
        case amongoc_server_errc_NewReplicaSetConfigurationIncompatible:
            return "NewReplicaSetConfigurationIncompatible";
        case amongoc_server_errc_NodeNotElectable:
            return "NodeNotElectable";
        case amongoc_server_errc_IncompatibleShardingMetadata:
            return "IncompatibleShardingMetadata";
        case amongoc_server_errc_DistributedClockSkewed:
            return "DistributedClockSkewed";
        case amongoc_server_errc_LockFailed:
            return "LockFailed";
        case amongoc_server_errc_InconsistentReplicaSetNames:
            return "InconsistentReplicaSetNames";
        case amongoc_server_errc_ConfigurationInProgress:
            return "ConfigurationInProgress";
        case amongoc_server_errc_CannotInitializeNodeWithData:
            return "CannotInitializeNodeWithData";
        case amongoc_server_errc_NotExactValueField:
            return "NotExactValueField";
        case amongoc_server_errc_WriteConflict:
            return "WriteConflict";
        case amongoc_server_errc_InitialSyncFailure:
            return "InitialSyncFailure";
        case amongoc_server_errc_InitialSyncOplogSourceMissing:
            return "InitialSyncOplogSourceMissing";
        case amongoc_server_errc_CommandNotSupported:
            return "CommandNotSupported";
        case amongoc_server_errc_DocTooLargeForCapped:
            return "DocTooLargeForCapped";
        case amongoc_server_errc_ConflictingOperationInProgress:
            return "ConflictingOperationInProgress";
        case amongoc_server_errc_NamespaceNotSharded:
            return "NamespaceNotSharded";
        case amongoc_server_errc_InvalidSyncSource:
            return "InvalidSyncSource";
        case amongoc_server_errc_OplogStartMissing:
            return "OplogStartMissing";
        case amongoc_server_errc_DocumentValidationFailure:
            return "DocumentValidationFailure";
        case amongoc_server_errc_NotAReplicaSet:
            return "NotAReplicaSet";
        case amongoc_server_errc_IncompatibleElectionProtocol:
            return "IncompatibleElectionProtocol";
        case amongoc_server_errc_CommandFailed:
            return "CommandFailed";
        case amongoc_server_errc_RPCProtocolNegotiationFailed:
            return "RPCProtocolNegotiationFailed";
        case amongoc_server_errc_UnrecoverableRollbackError:
            return "UnrecoverableRollbackError";
        case amongoc_server_errc_LockNotFound:
            return "LockNotFound";
        case amongoc_server_errc_LockStateChangeFailed:
            return "LockStateChangeFailed";
        case amongoc_server_errc_SymbolNotFound:
            return "SymbolNotFound";
        case amongoc_server_errc_FailedToSatisfyReadPreference:
            return "FailedToSatisfyReadPreference";
        case amongoc_server_errc_ReadConcernMajorityNotAvailableYet:
            return "ReadConcernMajorityNotAvailableYet";
        case amongoc_server_errc_StaleTerm:
            return "StaleTerm";
        case amongoc_server_errc_CappedPositionLost:
            return "CappedPositionLost";
        case amongoc_server_errc_IncompatibleShardingConfigVersion:
            return "IncompatibleShardingConfigVersion";
        case amongoc_server_errc_RemoteOplogStale:
            return "RemoteOplogStale";
        case amongoc_server_errc_JSInterpreterFailure:
            return "JSInterpreterFailure";
        case amongoc_server_errc_InvalidSSLConfiguration:
            return "InvalidSSLConfiguration";
        case amongoc_server_errc_SSLHandshakeFailed:
            return "SSLHandshakeFailed";
        case amongoc_server_errc_JSUncatchableError:
            return "JSUncatchableError";
        case amongoc_server_errc_CursorInUse:
            return "CursorInUse";
        case amongoc_server_errc_IncompatibleCatalogManager:
            return "IncompatibleCatalogManager";
        case amongoc_server_errc_PooledConnectionsDropped:
            return "PooledConnectionsDropped";
        case amongoc_server_errc_ExceededMemoryLimit:
            return "ExceededMemoryLimit";
        case amongoc_server_errc_ZLibError:
            return "ZLibError";
        case amongoc_server_errc_ReadConcernMajorityNotEnabled:
            return "ReadConcernMajorityNotEnabled";
        case amongoc_server_errc_NoConfigPrimary:
            return "NoConfigPrimary";
        case amongoc_server_errc_StaleEpoch:
            return "StaleEpoch";
        case amongoc_server_errc_OperationCannotBeBatched:
            return "OperationCannotBeBatched";
        case amongoc_server_errc_OplogOutOfOrder:
            return "OplogOutOfOrder";
        case amongoc_server_errc_ChunkTooBig:
            return "ChunkTooBig";
        case amongoc_server_errc_InconsistentShardIdentity:
            return "InconsistentShardIdentity";
        case amongoc_server_errc_CannotApplyOplogWhilePrimary:
            return "CannotApplyOplogWhilePrimary";
        case amongoc_server_errc_CanRepairToDowngrade:
            return "CanRepairToDowngrade";
        case amongoc_server_errc_MustUpgrade:
            return "MustUpgrade";
        case amongoc_server_errc_DurationOverflow:
            return "DurationOverflow";
        case amongoc_server_errc_MaxStalenessOutOfRange:
            return "MaxStalenessOutOfRange";
        case amongoc_server_errc_IncompatibleCollationVersion:
            return "IncompatibleCollationVersion";
        case amongoc_server_errc_CollectionIsEmpty:
            return "CollectionIsEmpty";
        case amongoc_server_errc_ZoneStillInUse:
            return "ZoneStillInUse";
        case amongoc_server_errc_InitialSyncActive:
            return "InitialSyncActive";
        case amongoc_server_errc_ViewDepthLimitExceeded:
            return "ViewDepthLimitExceeded";
        case amongoc_server_errc_CommandNotSupportedOnView:
            return "CommandNotSupportedOnView";
        case amongoc_server_errc_OptionNotSupportedOnView:
            return "OptionNotSupportedOnView";
        case amongoc_server_errc_InvalidPipelineOperator:
            return "InvalidPipelineOperator";
        case amongoc_server_errc_CommandOnShardedViewNotSupportedOnMongod:
            return "CommandOnShardedViewNotSupportedOnMongod";
        case amongoc_server_errc_TooManyMatchingDocuments:
            return "TooManyMatchingDocuments";
        case amongoc_server_errc_CannotIndexParallelArrays:
            return "CannotIndexParallelArrays";
        case amongoc_server_errc_TransportSessionClosed:
            return "TransportSessionClosed";
        case amongoc_server_errc_TransportSessionNotFound:
            return "TransportSessionNotFound";
        case amongoc_server_errc_TransportSessionUnknown:
            return "TransportSessionUnknown";
        case amongoc_server_errc_QueryPlanKilled:
            return "QueryPlanKilled";
        case amongoc_server_errc_FileOpenFailed:
            return "FileOpenFailed";
        case amongoc_server_errc_ZoneNotFound:
            return "ZoneNotFound";
        case amongoc_server_errc_RangeOverlapConflict:
            return "RangeOverlapConflict";
        case amongoc_server_errc_WindowsPdhError:
            return "WindowsPdhError";
        case amongoc_server_errc_BadPerfCounterPath:
            return "BadPerfCounterPath";
        case amongoc_server_errc_AmbiguousIndexKeyPattern:
            return "AmbiguousIndexKeyPattern";
        case amongoc_server_errc_InvalidViewDefinition:
            return "InvalidViewDefinition";
        case amongoc_server_errc_ClientMetadataMissingField:
            return "ClientMetadataMissingField";
        case amongoc_server_errc_ClientMetadataAppNameTooLarge:
            return "ClientMetadataAppNameTooLarge";
        case amongoc_server_errc_ClientMetadataDocumentTooLarge:
            return "ClientMetadataDocumentTooLarge";
        case amongoc_server_errc_ClientMetadataCannotBeMutated:
            return "ClientMetadataCannotBeMutated";
        case amongoc_server_errc_LinearizableReadConcernError:
            return "LinearizableReadConcernError";
        case amongoc_server_errc_IncompatibleServerVersion:
            return "IncompatibleServerVersion";
        case amongoc_server_errc_PrimarySteppedDown:
            return "PrimarySteppedDown";
        case amongoc_server_errc_MasterSlaveConnectionFailure:
            return "MasterSlaveConnectionFailure";
        case amongoc_server_errc_FailPointEnabled:
            return "FailPointEnabled";
        case amongoc_server_errc_NoShardingEnabled:
            return "NoShardingEnabled";
        case amongoc_server_errc_BalancerInterrupted:
            return "BalancerInterrupted";
        case amongoc_server_errc_ViewPipelineMaxSizeExceeded:
            return "ViewPipelineMaxSizeExceeded";
        case amongoc_server_errc_InvalidIndexSpecificationOption:
            return "InvalidIndexSpecificationOption";
        case amongoc_server_errc_ReplicaSetMonitorRemoved:
            return "ReplicaSetMonitorRemoved";
        case amongoc_server_errc_ChunkRangeCleanupPending:
            return "ChunkRangeCleanupPending";
        case amongoc_server_errc_CannotBuildIndexKeys:
            return "CannotBuildIndexKeys";
        case amongoc_server_errc_NetworkInterfaceExceededTimeLimit:
            return "NetworkInterfaceExceededTimeLimit";
        case amongoc_server_errc_ShardingStateNotInitialized:
            return "ShardingStateNotInitialized";
        case amongoc_server_errc_TimeProofMismatch:
            return "TimeProofMismatch";
        case amongoc_server_errc_ClusterTimeFailsRateLimiter:
            return "ClusterTimeFailsRateLimiter";
        case amongoc_server_errc_NoSuchSession:
            return "NoSuchSession";
        case amongoc_server_errc_InvalidUUID:
            return "InvalidUUID";
        case amongoc_server_errc_TooManyLocks:
            return "TooManyLocks";
        case amongoc_server_errc_StaleClusterTime:
            return "StaleClusterTime";
        case amongoc_server_errc_CannotVerifyAndSignLogicalTime:
            return "CannotVerifyAndSignLogicalTime";
        case amongoc_server_errc_KeyNotFound:
            return "KeyNotFound";
        case amongoc_server_errc_IncompatibleRollbackAlgorithm:
            return "IncompatibleRollbackAlgorithm";
        case amongoc_server_errc_DuplicateSession:
            return "DuplicateSession";
        case amongoc_server_errc_AuthenticationRestrictionUnmet:
            return "AuthenticationRestrictionUnmet";
        case amongoc_server_errc_DatabaseDropPending:
            return "DatabaseDropPending";
        case amongoc_server_errc_ElectionInProgress:
            return "ElectionInProgress";
        case amongoc_server_errc_IncompleteTransactionHistory:
            return "IncompleteTransactionHistory";
        case amongoc_server_errc_UpdateOperationFailed:
            return "UpdateOperationFailed";
        case amongoc_server_errc_FTDCPathNotSet:
            return "FTDCPathNotSet";
        case amongoc_server_errc_FTDCPathAlreadySet:
            return "FTDCPathAlreadySet";
        case amongoc_server_errc_IndexModified:
            return "IndexModified";
        case amongoc_server_errc_CloseChangeStream:
            return "CloseChangeStream";
        case amongoc_server_errc_IllegalOpMsgFlag:
            return "IllegalOpMsgFlag";
        case amongoc_server_errc_QueryFeatureNotAllowed:
            return "QueryFeatureNotAllowed";
        case amongoc_server_errc_TransactionTooOld:
            return "TransactionTooOld";
        case amongoc_server_errc_AtomicityFailure:
            return "AtomicityFailure";
        case amongoc_server_errc_CannotImplicitlyCreateCollection:
            return "CannotImplicitlyCreateCollection";
        case amongoc_server_errc_SessionTransferIncomplete:
            return "SessionTransferIncomplete";
        case amongoc_server_errc_MustDowngrade:
            return "MustDowngrade";
        case amongoc_server_errc_DNSHostNotFound:
            return "DNSHostNotFound";
        case amongoc_server_errc_DNSProtocolError:
            return "DNSProtocolError";
        case amongoc_server_errc_MaxSubPipelineDepthExceeded:
            return "MaxSubPipelineDepthExceeded";
        case amongoc_server_errc_TooManyDocumentSequences:
            return "TooManyDocumentSequences";
        case amongoc_server_errc_RetryChangeStream:
            return "RetryChangeStream";
        case amongoc_server_errc_InternalErrorNotSupported:
            return "InternalErrorNotSupported";
        case amongoc_server_errc_ForTestingErrorExtraInfo:
            return "ForTestingErrorExtraInfo";
        case amongoc_server_errc_CursorKilled:
            return "CursorKilled";
        case amongoc_server_errc_NotImplemented:
            return "NotImplemented";
        case amongoc_server_errc_SnapshotTooOld:
            return "SnapshotTooOld";
        case amongoc_server_errc_DNSRecordTypeMismatch:
            return "DNSRecordTypeMismatch";
        case amongoc_server_errc_ConversionFailure:
            return "ConversionFailure";
        case amongoc_server_errc_CannotCreateCollection:
            return "CannotCreateCollection";
        case amongoc_server_errc_IncompatibleWithUpgradedServer:
            return "IncompatibleWithUpgradedServer";
        case amongoc_server_errc_BrokenPromise:
            return "BrokenPromise";
        case amongoc_server_errc_SnapshotUnavailable:
            return "SnapshotUnavailable";
        case amongoc_server_errc_ProducerConsumerQueueBatchTooLarge:
            return "ProducerConsumerQueueBatchTooLarge";
        case amongoc_server_errc_ProducerConsumerQueueEndClosed:
            return "ProducerConsumerQueueEndClosed";
        case amongoc_server_errc_StaleDbVersion:
            return "StaleDbVersion";
        case amongoc_server_errc_StaleChunkHistory:
            return "StaleChunkHistory";
        case amongoc_server_errc_NoSuchTransaction:
            return "NoSuchTransaction";
        case amongoc_server_errc_ReentrancyNotAllowed:
            return "ReentrancyNotAllowed";
        case amongoc_server_errc_FreeMonHttpInFlight:
            return "FreeMonHttpInFlight";
        case amongoc_server_errc_FreeMonHttpTemporaryFailure:
            return "FreeMonHttpTemporaryFailure";
        case amongoc_server_errc_FreeMonHttpPermanentFailure:
            return "FreeMonHttpPermanentFailure";
        case amongoc_server_errc_TransactionCommitted:
            return "TransactionCommitted";
        case amongoc_server_errc_TransactionTooLarge:
            return "TransactionTooLarge";
        case amongoc_server_errc_UnknownFeatureCompatibilityVersion:
            return "UnknownFeatureCompatibilityVersion";
        case amongoc_server_errc_KeyedExecutorRetry:
            return "KeyedExecutorRetry";
        case amongoc_server_errc_InvalidResumeToken:
            return "InvalidResumeToken";
        case amongoc_server_errc_TooManyLogicalSessions:
            return "TooManyLogicalSessions";
        case amongoc_server_errc_ExceededTimeLimit:
            return "ExceededTimeLimit";
        case amongoc_server_errc_OperationNotSupportedInTransaction:
            return "OperationNotSupportedInTransaction";
        case amongoc_server_errc_TooManyFilesOpen:
            return "TooManyFilesOpen";
        case amongoc_server_errc_OrphanedRangeCleanUpFailed:
            return "OrphanedRangeCleanUpFailed";
        case amongoc_server_errc_FailPointSetFailed:
            return "FailPointSetFailed";
        case amongoc_server_errc_PreparedTransactionInProgress:
            return "PreparedTransactionInProgress";
        case amongoc_server_errc_CannotBackup:
            return "CannotBackup";
        case amongoc_server_errc_DataModifiedByRepair:
            return "DataModifiedByRepair";
        case amongoc_server_errc_RepairedReplicaSetNode:
            return "RepairedReplicaSetNode";
        case amongoc_server_errc_JSInterpreterFailureWithStack:
            return "JSInterpreterFailureWithStack";
        case amongoc_server_errc_MigrationConflict:
            return "MigrationConflict";
        case amongoc_server_errc_ProducerConsumerQueueProducerQueueDepthExceeded:
            return "ProducerConsumerQueueProducerQueueDepthExceeded";
        case amongoc_server_errc_ProducerConsumerQueueConsumed:
            return "ProducerConsumerQueueConsumed";
        case amongoc_server_errc_ExchangePassthrough:
            return "ExchangePassthrough";
        case amongoc_server_errc_IndexBuildAborted:
            return "IndexBuildAborted";
        case amongoc_server_errc_AlarmAlreadyFulfilled:
            return "AlarmAlreadyFulfilled";
        case amongoc_server_errc_UnsatisfiableCommitQuorum:
            return "UnsatisfiableCommitQuorum";
        case amongoc_server_errc_ClientDisconnect:
            return "ClientDisconnect";
        case amongoc_server_errc_ChangeStreamFatalError:
            return "ChangeStreamFatalError";
        case amongoc_server_errc_TransactionCoordinatorSteppingDown:
            return "TransactionCoordinatorSteppingDown";
        case amongoc_server_errc_TransactionCoordinatorReachedAbortDecision:
            return "TransactionCoordinatorReachedAbortDecision";
        case amongoc_server_errc_WouldChangeOwningShard:
            return "WouldChangeOwningShard";
        case amongoc_server_errc_ForTestingErrorExtraInfoWithExtraInfoInNamespace:
            return "ForTestingErrorExtraInfoWithExtraInfoInNamespace";
        case amongoc_server_errc_IndexBuildAlreadyInProgress:
            return "IndexBuildAlreadyInProgress";
        case amongoc_server_errc_ChangeStreamHistoryLost:
            return "ChangeStreamHistoryLost";
        case amongoc_server_errc_TransactionCoordinatorDeadlineTaskCanceled:
            return "TransactionCoordinatorDeadlineTaskCanceled";
        case amongoc_server_errc_ChecksumMismatch:
            return "ChecksumMismatch";
        case amongoc_server_errc_WaitForMajorityServiceEarlierOpTimeAvailable:
            return "WaitForMajorityServiceEarlierOpTimeAvailable";
        case amongoc_server_errc_TransactionExceededLifetimeLimitSeconds:
            return "TransactionExceededLifetimeLimitSeconds";
        case amongoc_server_errc_NoQueryExecutionPlans:
            return "NoQueryExecutionPlans";
        case amongoc_server_errc_QueryExceededMemoryLimitNoDiskUseAllowed:
            return "QueryExceededMemoryLimitNoDiskUseAllowed";
        case amongoc_server_errc_InvalidSeedList:
            return "InvalidSeedList";
        case amongoc_server_errc_InvalidTopologyType:
            return "InvalidTopologyType";
        case amongoc_server_errc_InvalidHeartBeatFrequency:
            return "InvalidHeartBeatFrequency";
        case amongoc_server_errc_TopologySetNameRequired:
            return "TopologySetNameRequired";
        case amongoc_server_errc_HierarchicalAcquisitionLevelViolation:
            return "HierarchicalAcquisitionLevelViolation";
        case amongoc_server_errc_InvalidServerType:
            return "InvalidServerType";
        case amongoc_server_errc_OCSPCertificateStatusRevoked:
            return "OCSPCertificateStatusRevoked";
        case amongoc_server_errc_RangeDeletionAbandonedBecauseCollectionWithUUIDDoesNotExist:
            return "RangeDeletionAbandonedBecauseCollectionWithUUIDDoesNotExist";
        case amongoc_server_errc_DataCorruptionDetected:
            return "DataCorruptionDetected";
        case amongoc_server_errc_OCSPCertificateStatusUnknown:
            return "OCSPCertificateStatusUnknown";
        case amongoc_server_errc_SplitHorizonChange:
            return "SplitHorizonChange";
        case amongoc_server_errc_ShardInvalidatedForTargeting:
            return "ShardInvalidatedForTargeting";
        case amongoc_server_errc_RangeDeletionAbandonedBecauseTaskDocumentDoesNotExist:
            return "RangeDeletionAbandonedBecauseTaskDocumentDoesNotExist";
        case amongoc_server_errc_CurrentConfigNotCommittedYet:
            return "CurrentConfigNotCommittedYet";
        case amongoc_server_errc_ExhaustCommandFinished:
            return "ExhaustCommandFinished";
        case amongoc_server_errc_PeriodicJobIsStopped:
            return "PeriodicJobIsStopped";
        case amongoc_server_errc_TransactionCoordinatorCanceled:
            return "TransactionCoordinatorCanceled";
        case amongoc_server_errc_OperationIsKilledAndDelisted:
            return "OperationIsKilledAndDelisted";
        case amongoc_server_errc_ResumableRangeDeleterDisabled:
            return "ResumableRangeDeleterDisabled";
        case amongoc_server_errc_ObjectIsBusy:
            return "ObjectIsBusy";
        case amongoc_server_errc_TooStaleToSyncFromSource:
            return "TooStaleToSyncFromSource";
        case amongoc_server_errc_QueryTrialRunCompleted:
            return "QueryTrialRunCompleted";
        case amongoc_server_errc_ConnectionPoolExpired:
            return "ConnectionPoolExpired";
        case amongoc_server_errc_ForTestingOptionalErrorExtraInfo:
            return "ForTestingOptionalErrorExtraInfo";
        case amongoc_server_errc_MovePrimaryInProgress:
            return "MovePrimaryInProgress";
        case amongoc_server_errc_TenantMigrationConflict:
            return "TenantMigrationConflict";
        case amongoc_server_errc_TenantMigrationCommitted:
            return "TenantMigrationCommitted";
        case amongoc_server_errc_APIVersionError:
            return "APIVersionError";
        case amongoc_server_errc_APIStrictError:
            return "APIStrictError";
        case amongoc_server_errc_APIDeprecationError:
            return "APIDeprecationError";
        case amongoc_server_errc_TenantMigrationAborted:
            return "TenantMigrationAborted";
        case amongoc_server_errc_OplogQueryMinTsMissing:
            return "OplogQueryMinTsMissing";
        case amongoc_server_errc_NoSuchTenantMigration:
            return "NoSuchTenantMigration";
        case amongoc_server_errc_TenantMigrationAccessBlockerShuttingDown:
            return "TenantMigrationAccessBlockerShuttingDown";
        case amongoc_server_errc_TenantMigrationInProgress:
            return "TenantMigrationInProgress";
        case amongoc_server_errc_SkipCommandExecution:
            return "SkipCommandExecution";
        case amongoc_server_errc_FailedToRunWithReplyBuilder:
            return "FailedToRunWithReplyBuilder";
        case amongoc_server_errc_CannotDowngrade:
            return "CannotDowngrade";
        case amongoc_server_errc_ServiceExecutorInShutdown:
            return "ServiceExecutorInShutdown";
        case amongoc_server_errc_MechanismUnavailable:
            return "MechanismUnavailable";
        case amongoc_server_errc_TenantMigrationForgotten:
            return "TenantMigrationForgotten";
        case amongoc_server_errc_SocketException:
            return "SocketException";
        case amongoc_server_errc_CannotGrowDocumentInCappedNamespace:
            return "CannotGrowDocumentInCappedNamespace";
        case amongoc_server_errc_NotWritablePrimary:
            return "NotWritablePrimary";
        case amongoc_server_errc_BSONObjectTooLarge:
            return "BSONObjectTooLarge";
        case amongoc_server_errc_DuplicateKey:
            return "DuplicateKey";
        case amongoc_server_errc_InterruptedAtShutdown:
            return "InterruptedAtShutdown";
        case amongoc_server_errc_Interrupted:
            return "Interrupted";
        case amongoc_server_errc_InterruptedDueToReplStateChange:
            return "InterruptedDueToReplStateChange";
        case amongoc_server_errc_BackgroundOperationInProgressForDatabase:
            return "BackgroundOperationInProgressForDatabase";
        case amongoc_server_errc_BackgroundOperationInProgressForNamespace:
            return "BackgroundOperationInProgressForNamespace";
        case amongoc_server_errc_MergeStageNoMatchingDocument:
            return "MergeStageNoMatchingDocument";
        case amongoc_server_errc_DatabaseDifferCase:
            return "DatabaseDifferCase";
        case amongoc_server_errc_StaleConfig:
            return "StaleConfig";
        case amongoc_server_errc_NotPrimaryNoSecondaryOk:
            return "NotPrimaryNoSecondaryOk";
        case amongoc_server_errc_NotPrimaryOrSecondary:
            return "NotPrimaryOrSecondary";
        case amongoc_server_errc_OutOfDiskSpace:
            return "OutOfDiskSpace";
        case amongoc_server_errc_ClientMarkedKilled:
            return "ClientMarkedKilled";
        default: {
            switch (ec) {
                // Added manually, not documented by the server reference:
            case 40571:
                return "OP_MSG requests require a $db argument";
            }
        }
        }
        return nullptr;
    }
} server_category_inst;

struct client_category_cls : std::error_category {
    const char* name() const noexcept override { return "amongoc.client"; }
    std::string message(int ec) const noexcept override {
        char buf[128];
        return this->message_noalloc(ec, buf, sizeof buf);
    }

    const char* message_noalloc(int ec, char* buf, size_t buflen) const noexcept {
        switch (static_cast<::amongoc_client_errc>(ec)) {
        case amongoc_client_errc_okay:
            return "no error";
        case amongoc_client_errc_invalid_update_document:
            return "invalid document for an ‘update’ operation";
        default:
            std::snprintf(buf, buflen, "amongoc.client:%d", ec);
            return buf;
        }
    }
} client_category_inst;

struct tls_category_cls : std::error_category {
    const char* name() const noexcept override { return "amongoc.tls"; }
    std::string message(int ec) const noexcept override {
        const char* msg = ::ERR_error_string(static_cast<unsigned long>(ec), nullptr);
        return msg ? msg : fmt::format("amongoc.tls:{}", ec);
    }

    const char* message_noalloc(int ec, char* buf, size_t buflen) const noexcept {
        const char* msg = ::ERR_error_string(static_cast<unsigned long>(ec), nullptr);
        if (msg) {
            return msg;
        }
        std::snprintf(buf, buflen, "amongoc.tls:%d", ec);
        return buf;
    }
} tls_category_inst;

const char* get_msg(const auto& cat, int c, char* buf, size_t buflen) {
    std::string msg = cat.message(c);
    std::snprintf(buf, buflen, "%*s", (int)msg.size(), msg.data());
    return buf;
}

const char* get_msg(const auto& cat, int c, char* buf, size_t buflen)
    requires requires(const char* msg) { msg = cat.message_noalloc(c, buf, buflen); }
{
    return cat.message_noalloc(c, buf, buflen);
}

// Inherit some status attributes from a C++ category
template <auto GetCategory>
struct from_cxx_category {
    static const char* name() noexcept { return GetCategory().name(); }
    static const char* message(int c, char* buf, size_t buflen) noexcept {
        return get_msg(GetCategory(), c, buf, buflen);
    }
};

struct generic_category_attrs : from_cxx_category<std::generic_category> {
    static bool        is_timeout(int e) { return e == ETIMEDOUT; }
    static bool        is_cancellation(int e) { return e == ECANCELED; }
    static const char* message(int c, char*, size_t) noexcept { return std::strerror(c); }
};
struct system_category_attrs : from_cxx_category<std::system_category> {
    static bool        is_timeout(int e) { return e == ETIMEDOUT; }
    static bool        is_cancellation(int e) { return e == ECANCELED; }
    static const char* message(int c, char*, size_t) noexcept { return std::strerror(c); }
};

struct netdb_category_attrs : from_cxx_category<asio::error::get_netdb_category> {};
struct addrinfo_category_attrs : from_cxx_category<asio::error::get_addrinfo_category> {};
struct io_category_attrs : from_cxx_category<amongoc::io_category> {};
struct server_category_attrs : from_cxx_category<amongoc::server_category> {};
struct client_category_attrs : from_cxx_category<amongoc::client_category> {};
struct tls_category_attrs : from_cxx_category<amongoc::tls_category> {};
struct unknown_category_attrs : from_cxx_category<amongoc::unknown_category> {};

template <typename>
constexpr auto get_is_error = nullptr;
template <typename T>
    requires requires { T::is_error(42); }
constexpr auto get_is_error<T> = &T::is_error;
template <typename>
constexpr auto get_is_timeout = nullptr;
template <typename T>
    requires requires { T::is_timeout(42); }
constexpr auto get_is_timeout<T> = &T::is_timeout;
template <typename>
constexpr auto get_is_cancellation = nullptr;
template <typename T>
    requires requires { T::is_cancellation(42); }
constexpr auto get_is_cancellation<T> = &T::is_cancellation;

}  // namespace

#define X(Ident)                                                                                   \
    DefCategory(Ident, MLIB_PASTE_3(amongoc_, Ident, _category), MLIB_PASTE(Ident, _category_attrs))
#define DefCategory(Ident, GlobalName, Attrs)                                                      \
    /* Definition of the category global instance */                                               \
    constexpr ::amongoc_status_category_vtable MLIB_PASTE_3(amongoc_, Ident, _category) = {        \
        .name            = &Attrs::name,                                                           \
        .message         = &Attrs::message,                                                        \
        .is_error        = get_is_error<Attrs>,                                                    \
        .is_cancellation = get_is_cancellation<Attrs>,                                             \
        .is_timeout      = get_is_timeout<Attrs>,                                                  \
    };
AMONGOC_STATUS_CATEGORY_X_LIST();
#undef X
#undef DefCategory

std::string status::message() const noexcept {
    amongoc_declmsg(msg, *this);
    return msg;
}

status status::from(const std::error_code& ec) noexcept {
    if (ec.category() == asio::error::get_misc_category()) {
        // TODO: Convert these to more comprehensible errors.
        switch (static_cast<asio::error::misc_errors>(ec.value())) {
        case asio::error::already_open:
            return {&amongoc_generic_category, EISCONN};
        case asio::error::eof:
            return {&amongoc_io_category, amongoc_errc_connection_closed};
        case asio::error::not_found:
            return {&amongoc_generic_category, ENOENT};
        case asio::error::fd_set_failure:
            return {&amongoc_generic_category, EINVAL};
            break;
        }
    }
#define X(Ident)                                                                                   \
    if (ec.category() == amongoc::MLIB_PASTE(Ident, _category)()) {                                \
        return {&MLIB_PASTE_3(amongoc_, Ident, _category), ec.value()};                            \
    }
    AMONGOC_STATUS_CATEGORY_X_LIST()
#undef X
    return {&amongoc_unknown_category, ec.value()};
}

std::error_code status::as_error_code() const noexcept {
#define X(Ident)                                                                                   \
    if (this->category == &::MLIB_PASTE_3(amongoc_, Ident, _category)) {                           \
        return std::error_code(this->code, amongoc::MLIB_PASTE(Ident, _category)());               \
    }
    AMONGOC_STATUS_CATEGORY_X_LIST()
#undef X
    return std::error_code(this->code, unknown_category_inst);
}

#define DECL_CXX_CAT_GETTER(Ident, CxxExpression)                                                  \
    std::error_category const& amongoc::MLIB_PASTE(Ident, _category)() noexcept {                  \
        return CxxExpression;                                                                      \
    }                                                                                              \
    static_assert(true)
DECL_CXX_CAT_GETTER(generic, std::generic_category());
DECL_CXX_CAT_GETTER(system, std::system_category());
DECL_CXX_CAT_GETTER(netdb, asio::error::get_netdb_category());
DECL_CXX_CAT_GETTER(addrinfo, asio::error::get_addrinfo_category());
DECL_CXX_CAT_GETTER(io, io_category_inst);
DECL_CXX_CAT_GETTER(server, server_category_inst);
DECL_CXX_CAT_GETTER(client, client_category_inst);
DECL_CXX_CAT_GETTER(tls, tls_category_inst);
DECL_CXX_CAT_GETTER(unknown, unknown_category_inst);
#undef DECL_CXX_CAT_GETTER

::amongoc_tls_errc amongoc_status_tls_reason(amongoc_status st) noexcept {
    if (st.category != &::amongoc_tls_category) {
        return ::amongoc_tls_errc_okay;
    }
    return static_cast<::amongoc_tls_errc>(::ERR_GET_REASON(static_cast<unsigned long>(st.code)));
}
