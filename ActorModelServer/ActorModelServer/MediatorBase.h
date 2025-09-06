#pragma once
#include <chrono>
#include <unordered_map>
#include <functional>
#include "NonNetworkActor.h"

using TransactionIdType = uint64_t;
using ParticipantIdType = uint64_t;

enum class TransactionState
{
	PENDING,
	PREPARING,
	COMMITTING,
	COMPLETED,
	FAILED,
	ROLLING_BACK
};

enum class ParticipantState
{
	WAITING,
	PREPARED,
	COMMITTED,
	FAILED
};

class MediatorBase : public NonNetworkActor
{
public:
	MediatorBase()
		: NonNetworkActor(false)
	{
	}
	~MediatorBase() override = default;

protected:
	struct Participant
	{
		Participant() = default;
		explicit Participant(const ParticipantIdType participantId)
			: id(participantId)
		{
		}

		ParticipantIdType id;
		ParticipantState state{ ParticipantState::WAITING };
		std::function<void()> prepareAction;
		std::function<void()> commitAction;
		std::function<void()> rollbackAction;
		std::chrono::steady_clock::time_point lastUpdate;
	};

	struct Transaction
	{
		TransactionIdType id;
		TransactionState state = TransactionState::PENDING;
		std::unordered_map<ParticipantIdType, Participant> participants;
		std::chrono::steady_clock::time_point startTime;
		std::chrono::microseconds timeout{ 5000 };
	};

	std::unordered_map<TransactionIdType, Transaction> activeTransactions;
	static inline std::atomic<TransactionIdType> transactionIdGenerator{ 1 };

public:
    TransactionIdType StartTransaction(const std::vector<ParticipantIdType>& participantIds, std::chrono::seconds timeout = std::chrono::seconds{ 30 });
    void OnParticipantReady(TransactionIdType transactionId, ParticipantIdType participantId, bool success);

protected:
    virtual void OnTransactionStarted(TransactionIdType transactionId) = 0;
    virtual void OnTransactionCompleted(TransactionIdType transactionId) = 0;
    virtual void OnTransactionFailed(TransactionIdType transactionId) = 0;

    virtual void SendPrepareRequest(TransactionIdType transactionId, const Participant& participant) = 0;
    virtual void SendCommitRequest(TransactionIdType transactionId, const Participant& participant) = 0;
    virtual void SendRollbackRequest(TransactionIdType transactionId, const Participant& participant) = 0;

private:
    static TransactionIdType GenerateTransactionId();
    static bool AllParticipantsPrepared(const Transaction& transaction);

	void CommitTransaction(TransactionIdType transactionId);
	void RollbackTransaction(TransactionIdType transactionId);
    void OnTimer() override;
};