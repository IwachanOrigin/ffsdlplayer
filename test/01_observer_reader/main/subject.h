
#ifndef SUBJECT_H_
#define SUBJECT_H_

#include <vector>
#include <memory>

namespace player
{
class Observer;

class Subject
{

public:
  explicit Subject() = default;
  virtual ~Subject() = default;

  void addObserver(std::shared_ptr<Observer> observer);
  void deleteObserver(std::shared_ptr<Observer> observer);

protected:
  void notifyObservers();

private:
  std::vector<std::shared_ptr<Observer>> m_observers;

};

} // player

#endif // SUBJECT_H_

