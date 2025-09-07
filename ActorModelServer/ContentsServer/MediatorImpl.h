#pragma once
#include "MediatorBase.h"

class TradeMediator : public MediatorBase
{
public:
	TradeMediator() = default;
	~TradeMediator() override = default;

	struct TradeRequest
	{
		ParticipantIdType buyerId;
		ParticipantIdType sellerId;
		int64_t money;
		int64_t itemId;
	};

public:
	bool RequestTrade(const TradeRequest& request);

protected:
	void OnTransactionCompleted(TransactionIdType transactionId) override;
	void OnTransactionFailed(TransactionIdType transactionId) override;
	void OnTransactionStarted(TransactionIdType transactionId) override;

	void SendPrepareRequest(TransactionIdType transactionId, const Participant& participant) override;
	void SendCommitRequest(TransactionIdType transactionId, const Participant& participant) override;
	void SendRollbackRequest(TransactionIdType transactionId, const Participant& participant) override;
};