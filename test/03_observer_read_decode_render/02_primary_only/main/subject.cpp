
#include "observer.h"
#include "subject.h"

using namespace player;

void Subject::addObserver(Observer* observer)
{
  auto itr = std::find(m_observers.begin(), m_observers.end(), observer);
  if (itr == m_observers.end())
  {
    m_observers.push_back(observer);
  }
}

void Subject::deleteObserver(Observer* observer)
{
  auto itr = std::find(m_observers.begin(), m_observers.end(), observer);
  if (itr != m_observers.end())
  {
    m_observers.erase(itr);
  }
}

void Subject::notifyObservers()
{
  for(auto observer : m_observers)
  {
    observer->update(this);
  }
}
