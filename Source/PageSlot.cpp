#include "PageSlot.h"

#include <assert.h>

using namespace cache;

PageSlot::PageSlot()
{
	state = STATE_FREE;
	page = INVALID_PAGE;
	unloadPage = INVALID_PAGE;
	isDirty = false;
	int capturedNumber = 0;
	int waitingNumber = 0;
}

void PageSlot::reset()
{
	state = STATE_FREE;
	page = INVALID_PAGE;
	unloadPage = INVALID_PAGE;
	isDirty = false;
}

bool PageSlot::isAvailable() const
{
	return (state == STATE_FREE || state == STATE_READY) && (waitingNumber_ == 0);
}

bool PageSlot::isPageUnload(PageNumber checkingPage)
{
	return state == STATE_UNLOAD && unloadPage == checkingPage;
}

bool PageSlot::isLoading() const
{
	return state == STATE_UNLOAD || state == STATE_LOAD;
}

bool PageSlot::canFlush() const
{
	return state == STATE_READY && isDirty == true;
}

void PageSlot::addCapture()
{
	capturedNumber_++;
}

void PageSlot::releaseCapture()
{
	assert(capturedNumber_ > 0); //software error: 'releaseCapture' was called without previous 'addCapture' call
	capturedNumber_--;

	if (capturedNumber_ == 0)
	{
		cvCapture_.notify_all();
	}
}

unsigned int PageSlot::getCaptureCount() const
{
	return capturedNumber_;
}

unsigned int PageSlot::getWaitingCount() const
{
	return waitingNumber_;
}

void PageSlot::waitCaptureFree(locker_t& locker)
{
	cvCapture_.wait(locker, [this]()
	{
		return this->capturedNumber_ == 0;
	});
}

void PageSlot::waitUnload(locker_t& locker)
{
	waitingNumber_++;

	cvUnload_.wait(locker,[this]() 
	{
		return this->state != PageSlot::STATE_UNLOAD;
	});

	waitingNumber_--;

	std::exception_ptr currentException = exception_;

	if (waitingNumber_ == 0)
	{
		exception_ = nullptr;
	}

	if (currentException)
	{
		std::rethrow_exception(currentException);
	}
}

void PageSlot::waitLoad(locker_t& locker)
{
	waitingNumber_++;

	cvLoad_.wait(locker, [this]()
	{
		return !this->isLoading();
	});

	waitingNumber_--;

	std::exception_ptr currentException = exception_;

	if (waitingNumber_ == 0)
	{
		exception_ = nullptr;
	}

	if (currentException)
	{
		std::rethrow_exception(currentException);
	}
}

void PageSlot::notifyUnload()
{
	cvUnload_.notify_all();
}

void PageSlot::notifyLoad()
{
	cvLoad_.notify_all();
}

void PageSlot::notifyException(std::exception_ptr exception)
{
	if (waitingNumber_ > 0)
	{
		exception_ = exception;
	}

	cvUnload_.notify_all();
	cvLoad_.notify_all();
}
