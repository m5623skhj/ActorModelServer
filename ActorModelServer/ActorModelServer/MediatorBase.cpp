#include "PreCompile.h"
#include "MediatorBase.h"
#include <ranges>
#include <algorithm>

TransactionIdType MediatorBase::StartTransaction(const std::vector<ParticipantIdType>& participantIds, const std::chrono::microseconds timeout)
{
    const auto transactionId = GenerateTransactionId();
    auto& transaction = activeTransactions[transactionId];

    transaction.id = transactionId;
    transaction.startTime = std::chrono::steady_clock::now();
    transaction.timeout = timeout;

    for (const auto& participantId : participantIds)
    {
        transaction.participants[participantId] = Participant{ participantId };
    }

    OnTransactionStarted(transactionId);
    return transactionId;
}

void MediatorBase::OnParticipantReady(const TransactionIdType transactionId, const ParticipantIdType participantId, const bool success)
{
    const auto itor = activeTransactions.find(transactionId);
    if (itor == activeTransactions.end())
    {
        return;
    }

    auto& transaction = itor->second;
    const auto participantIt = transaction.participants.find(participantId);
    if (participantIt == transaction.participants.end())
    {
        return;
    }

    auto& participant = participantIt->second;
    participant.lastUpdate = std::chrono::steady_clock::now();

    if (success)
    {
        participant.state = PARTICIPANT_STATE::PREPARED;

        if (AllParticipantsPrepared(transaction)) {
            CommitTransaction(transactionId);
        }
    }
    else
    {
        participant.state = PARTICIPANT_STATE::FAILED;
        RollbackTransaction(transactionId);
    }
}

TransactionIdType MediatorBase::GenerateTransactionId()
{
	return ++transactionIdGenerator;
}

bool MediatorBase::AllParticipantsPrepared(const Transaction& transaction)
{
	return std::ranges::all_of(transaction.participants, [](const auto& pair)
		{
			return pair.second.state == PARTICIPANT_STATE::PREPARED;
		});
}

void MediatorBase::CommitTransaction(const TransactionIdType transactionId)
{
    auto& transaction = activeTransactions[transactionId];
    transaction.state = MEDIATOR_TRANSACTION_STATE::COMMITTING;

    for (auto& participant : transaction.participants | std::views::values)
    {
        SendCommitRequest(transactionId, participant);
        participant.state = PARTICIPANT_STATE::COMMITTED;
    }

    transaction.state = MEDIATOR_TRANSACTION_STATE::COMPLETED;
    OnTransactionCompleted(transactionId);

    activeTransactions.erase(transactionId);
}

void MediatorBase::RollbackTransaction(const TransactionIdType transactionId)
{
    auto& transaction = activeTransactions[transactionId];
    transaction.state = MEDIATOR_TRANSACTION_STATE::ROLLING_BACK;

    for (auto& participant : transaction.participants | std::views::values)
    {
        if (participant.state == PARTICIPANT_STATE::PREPARED) {
            SendRollbackRequest(transactionId, participant);
        }
    }

    transaction.state = MEDIATOR_TRANSACTION_STATE::FAILED;
    OnTransactionFailed(transactionId);

    activeTransactions.erase(transactionId);
}

void MediatorBase::OnTimer()
{
    const auto now = std::chrono::steady_clock::now();
    std::vector<TransactionIdType> timedOutTransactions;

    for (const auto& [transactionId, transaction] : activeTransactions)
    {
        if (now - transaction.startTime > transaction.timeout)
        {
            timedOutTransactions.push_back(transactionId);
        }
    }

    for (const auto transactionId : timedOutTransactions)
    {
        RollbackTransaction(transactionId);
    }
}
