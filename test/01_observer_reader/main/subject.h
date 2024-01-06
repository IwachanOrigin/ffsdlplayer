
#ifndef SUBJECT_H_
#define SUBJECT_H_

#include <vector>

namespace player
{
class Observer;

class Subject
{

public:
  explicit Subject() = default;
  virtual ~Subject() = default;

  void addObserver(Observer* observer);
  void deleteObserver(Observer* observer);

protected:
  void notifyObservers();

private:
  std::vector<Observer*> m_observers;

};

} // player

#endif // SUBJECT_H_

