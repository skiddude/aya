#pragma once

#include "DataModel/ContentProvider.hpp"

namespace Aya
{

class AsyncResult
{
public:
    AsyncResult()
        : reqResult(Aya::AsyncHttpQueue::Succeeded){};

    // make result always more restrictive only.
    // Succeeded < Waiting < Failed.
    void returnResult(Aya::AsyncHttpQueue::RequestResult reqResult)
    {
        switch (reqResult)
        {
        case Aya::AsyncHttpQueue::Succeeded:
            break;
        case Aya::AsyncHttpQueue::Waiting:
            if (this->reqResult == Aya::AsyncHttpQueue::Succeeded)
            {
                this->reqResult = reqResult;
            }
            break;
        case Aya::AsyncHttpQueue::Failed:
            this->reqResult = reqResult;
            break;
        }
    }

    void returnWaitingFor(const Aya::ContentId& id)
    {
        returnResult(Aya::AsyncHttpQueue::Waiting);
        waitingFor.push_back(id);
    }

    Aya::AsyncHttpQueue::RequestResult reqResult;
    std::vector<Aya::ContentId> waitingFor;
};


} // namespace Aya