
/*
 Copyright (C) 2002 Ferdinando Ametrano

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it under the
 terms of the QuantLib license.  You should have received a copy of the
 license along with this program; if not, please email ferdinando@ametrano.net
 The license is also available online at http://quantlib.org/html/license.html

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/
/*! \file blackvariancecurve.hpp
    \brief Black volatility curve modelled as variance curve

  \fullpath
    ql/Volatilities/%blackvariancecurve.hpp
*/

// $Id$

#ifndef quantlib_blackvariancecurve_hpp
#define quantlib_blackvariancecurve_hpp

#include <ql/voltermstructure.hpp>
#include <vector>

namespace QuantLib {

    namespace VolTermStructures {

        //! Black volatility curve modelled as variance curve
        /*! This class calculates time dependant Black volatilities
            using a matrix of Black volatilities observed in the market as
            input.

            The calculation is performed interpolating on the variance curve.

            For dependance see BlackVarianceCurve
        */
        template<class Interpolator1D>
        class BlackVarianceCurve : public VarianceTermStructure,
                                   public Patterns::Observer {
          public:
            BlackVarianceCurve(const Date& referenceDate,
                               const DayCounter& dayCounter,
                               const std::vector<Date>& dates,
                               const std::vector<double>& blackVolCurve,
                               const std::string& underlying = "");
            Date referenceDate() const { return referenceDate_; }
            DayCounter dayCounter() const { return dayCounter_; }
            Date minDate() const { return referenceDate_; }
            Date maxDate() const { return maxDate_; }
            Time minTime() const { return 0.0; }
            Time maxTime() const { return times_.back(); }
            std::string underlying() const { return underlying_; }
            // Observer interface
            void update();
          protected:
            virtual double blackVarianceImpl(Time t, double,
                bool extrapolate = false) const;
          private:
            Date referenceDate_;
            DayCounter dayCounter_;
            Date maxDate_;
            std::string underlying_;
            std::vector<Time> times_;
            std::vector<double> variances_;
            Handle < Interpolator1D> varianceSurface_;
        };


        template<class Interpolator1D>
        BlackVarianceCurve<Interpolator1D>::BlackVarianceCurve(
            const Date& referenceDate,
            const DayCounter& dayCounter,
            const std::vector<Date>& dates,
            const std::vector<double>& blackVolCurve,
            const std::string& underlying)
        : referenceDate_(referenceDate), dayCounter_(dayCounter),
          maxDate_(dates.back()), underlying_(underlying) {

            QL_REQUIRE(dates.size()==blackVolCurve.size(),
                "mismatch between date vector and black vol vector");

            // cannot have dates[0]==referenceDate, since the
            // value of the vol at dates[0] would be lost
            // (variance at referenceDate must be zero)
            QL_REQUIRE(dates[0]>referenceDate,
                "cannot have dates[0]<=referenceDate");

            variances_ = std::vector<double>(dates.size());
            times_ = std::vector<Time>(dates.size());
            Size j;
            for (j=0; j<blackVolCurve.size(); j++) {
                times_[j] = dayCounter_.yearFraction(referenceDate, dates[j]);
                QL_REQUIRE(j==0 || times_[j]>times_[j-1],
                    "dates must be sorted unique!");
                variances_[j] = times_[j] *
                    blackVolCurve[j]*blackVolCurve[j];
            }
            varianceSurface_ = Handle<Interpolator1D> (new
                Interpolator1D(times_.begin(), times_.end(),
                variances_.begin()));
        }


        template<class Interpolator1D>
        void BlackVarianceCurve<Interpolator1D>::update() {
            notifyObservers();
        }

        template<class Interpolator1D>
        double BlackVarianceCurve<Interpolator1D>::
            blackVarianceImpl(Time t, double, bool extrapolate) const {

            QL_REQUIRE(t>=0.0,
                "BlackVarianceCurve::blackVarianceImpl : "
                "negative time (" + DoubleFormatter::toString(t) +
                ") not allowed");

            if (t<=times_[0])
                return (*varianceSurface_)(times_[0], extrapolate)*
                    t/times_[0];
            else if (t<=times_.back())
                return (*varianceSurface_)(t, extrapolate);
            else // t>times_.back() || extrapolate
                QL_REQUIRE(extrapolate,
                    "ConstantVol::blackVolImpl : "
                    "time (" + DoubleFormatter::toString(t) +
                    ") greater than max time (" +
                    DoubleFormatter::toString(times_.back()) +
                    ")");
                return (*varianceSurface_)(times_.back(), extrapolate)*
                    t/times_.back();
        }

    }

}


#endif
