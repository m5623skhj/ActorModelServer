#include "PreCompile.h"
#include "MediatorImpl.h"
#include "ServerCore.h"
#include "Player.h"

bool TradeMediator::RequestTrade(const TradeRequest& request)
{
	const auto transactionId = StartTransaction({ request.buyerId, request.sellerId }, std::chrono::seconds(30));
	const auto seller = ServerCore::GetInst().FindActor<Player>(request.sellerId, false);
	if (seller == nullptr)
	{
		return false;
	}

	const auto buyer = ServerCore::GetInst().FindActor<Player>(request.buyerId, false);
	if (buyer == nullptr)
	{
		return false;
	}

	if (not SendMessage(&Player::PrepareItemForTrade, seller, request.itemId) ||
		not SendMessage(&Player::PrepareMoneyForTrade, buyer, request.money))
	{
		RollbackTransaction(transactionId);
		return false;
	}

	return true;
}

void TradeMediator::OnTransactionCompleted(TransactionIdType transactionId)
{
}

void TradeMediator::OnTransactionFailed(TransactionIdType transactionId)
{
}

void TradeMediator::OnTransactionStarted(TransactionIdType transactionId)
{
}

void TradeMediator::SendPrepareRequest(TransactionIdType transactionId, const Participant& participant)
{
}

void TradeMediator::SendCommitRequest(TransactionIdType transactionId, const Participant& participant)
{
}

void TradeMediator::SendRollbackRequest(TransactionIdType transactionId, const Participant& participant)
{
}

