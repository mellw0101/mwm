#ifndef MWM_ANIMATOR_HPP
#define MWM_ANIMATOR_HPP

#include <xcb/xcb.h>
#include <thread>
#include <chrono>
#include <atomic>

#include "client.hpp"
#include "structs.hpp"

class Mwm_Animator /**
 *
 * @class XCPPBAnimator
 * @brief Class for animating the position and size of an XCB window.
 *
 */
{
    public: // variables
        Mwm_Animator(const uint32_t & window)
        : window(window) {}

        Mwm_Animator(client * c)
        : c(c) {}
    ;
    public: // destructor
        ~Mwm_Animator() /**
         * @brief Destructor to ensure the animation threads are stopped when the object is destroyed.
         */
        {
            stopAnimations();
        }
    ;
    public: // methods
        void animate(int startX, int startY, int startWidth, int startHeight, int endX, int endY, int endWidth, int endHeight, int duration) /**
         * @brief Animates the position and size of an object from a starting point to an ending point.
         * 
         * @param startX The starting X coordinate.
         * @param startY The starting Y coordinate.
         * @param startWidth The starting width.
         * @param startHeight The starting height.
         * @param endX The ending X coordinate.
         * @param endY The ending Y coordinate.
         * @param endWidth The ending width.
         * @param endHeight The ending height.
         * @param duration The duration of the animation in milliseconds.
         */
        {
            /* ENSURE ANY EXISTING ANIMATION IS STOPPED */
            stopAnimations();
            
            /* INITILIZE CLASS VARIABELS WITH INPUT VALUES */
            currentX      = startX;
            currentY      = startY;
            currentWidth  = startWidth;
            currentHeight = startHeight;

            int steps = duration; 

            /**
             * @brief Calculate if the step is positive or negative for each property.
             *
             * The variables @param stepX, stepY, stepWidth, stepHeight are always set to either 1 or -1.
             * This is determined by dividing the absolute value of the difference between the start and end values
             * by the difference itself. This results in a value of 1 or -1, which is used to determine if the animation 
             * is moving in a positive (increasing) or negative (decreasing) direction for each property.
             */
            stepX      = std::abs(endX - startX)           / (endX - startX);
            stepY      = std::abs(endY - startY)           / (endY - startY);
            stepWidth  = std::abs(endWidth - startWidth)   / (endWidth - startWidth);
            stepHeight = std::abs(endHeight - startHeight) / (endHeight - startHeight);

            /**
             * @brief CALCULATE THE DURATION FOR EACH STEP BASED ON THE TOTAL ANIMATION DURATION AND THE ABSOLUTE VALUE OF THE LENGTH OF EACH ANIMATION 
             * 
             * @param XAnimDuration, YAnimDuration, WAnimDuration, HAnimDuration represent the time each step takes to iterate one pixel for each respective thread.
             * 
             * The duration for each step is calculated by dividing the total animation duration by the absolute value of the lengt on the respective animation.
             * This ensures that each thread will iterate each pixel from start to end value,
             * ensuring that all threads will complete at the same time.
             */
            XAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endX - startX));
            YAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endY - startY)); 
            WAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endWidth - startWidth));
            HAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endHeight - startHeight)); 

            /* START ANIMATION THREADS */
            XAnimationThread = std::thread(&Mwm_Animator::XAnimation, this, endX);
            YAnimationThread = std::thread(&Mwm_Animator::YAnimation, this, endY);
            WAnimationThread = std::thread(&Mwm_Animator::WAnimation, this, endWidth);
            HAnimationThread = std::thread(&Mwm_Animator::HAnimation, this, endHeight);

            /* WAIT FOR ANIMATION TO COMPLETE */
            std::this_thread::sleep_for(std::chrono::milliseconds(duration));

            /* STOP THE ANIMATION */
            stopAnimations();
        }
        void animate_client(int startX, int startY, int startWidth, int startHeight, int endX, int endY, int endWidth, int endHeight, int duration)
        {
            /* ENSURE ANY EXISTING ANIMATION IS STOPPED */
            stopAnimations();
            
            /* INITILIZE CLASS VARIABELS WITH INPUT VALUES */
            currentX      = startX;
            currentY      = startY;
            currentWidth  = startWidth;
            currentHeight = startHeight;

            int steps = duration; 

            /**
             * @brief Calculate if the step is positive or negative for each property.
             *
             * The variables @param stepX, stepY, stepWidth, stepHeight are always set to either 1 or -1.
             * This is determined by dividing the absolute value of the difference between the start and end values
             * by the difference itself. This results in a value of 1 or -1, which is used to determine if the animation 
             * is moving in a positive (increasing) or negative (decreasing) direction for each property.
             */
            stepX      = std::abs(endX - startX)           / (endX - startX);
            stepY      = std::abs(endY - startY)           / (endY - startY);
            stepWidth  = std::abs(endWidth - startWidth)   / (endWidth - startWidth);
            stepHeight = std::abs(endHeight - startHeight) / (endHeight - startHeight);

            /**
             * @brief CALCULATE THE DURATION FOR EACH STEP BASED ON THE TOTAL ANIMATION DURATION AND THE ABSOLUTE VALUE OF THE LENGTH OF EACH ANIMATION 
             * 
             * @param XAnimDuration, YAnimDuration, WAnimDuration, HAnimDuration represent the time each step takes to iterate one pixel for each respective thread.
             * 
             * The duration for each step is calculated by dividing the total animation duration by the absolute value of the lengt on the respective animation.
             * This ensures that each thread will iterate each pixel from start to end value,
             * ensuring that all threads will complete at the same time.
             */
            XAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endX - startX));
            YAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endY - startY)); 
            WAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endWidth - startWidth));
            HAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endHeight - startHeight)); 
            GAnimDuration = frameDuration;

            /* START ANIMATION THREADS */
            GAnimationThread = std::thread(&Mwm_Animator::GFrameAnimation, this, endX, endY, endWidth, endHeight);
            XAnimationThread = std::thread(&Mwm_Animator::CliXAnimation, this, endX);
            YAnimationThread = std::thread(&Mwm_Animator::CliYAnimation, this, endY);
            WAnimationThread = std::thread(&Mwm_Animator::CliWAnimation, this, endWidth);
            HAnimationThread = std::thread(&Mwm_Animator::CliHAnimation, this, endHeight);

            /* WAIT FOR ANIMATION TO COMPLETE */
            std::this_thread::sleep_for(std::chrono::milliseconds(duration));

            /* STOP THE ANIMATION */
            stopAnimations();
        }
        enum DIRECTION 
        {
            NEXT,
            PREV
        };
        void animate_client_x(int startX, int endX, int duration)
        {
            /* ENSURE ANY EXISTING ANIMATION IS STOPPED */
            stopAnimations();
            
            /* INITILIZE CLASS VARIABELS WITH INPUT VALUES */
            currentX = startX;

            int steps = duration; 

            /**
             * @brief Calculate if the step is positive or negative for each property.
             *
             * The variables @param stepX, stepY, stepWidth, stepHeight are always set to either 1 or -1.
             * This is determined by dividing the absolute value of the difference between the start and end values
             * by the difference itself. This results in a value of 1 or -1, which is used to determine if the animation 
             * is moving in a positive (increasing) or negative (decreasing) direction for each property.
             */
            stepX = std::abs(endX - startX) / (endX - startX);

            /**
             * @brief CALCULATE THE DURATION FOR EACH STEP BASED ON THE TOTAL ANIMATION DURATION AND THE ABSOLUTE VALUE OF THE LENGTH OF EACH ANIMATION 
             * 
             * @param XAnimDuration, YAnimDuration, WAnimDuration, HAnimDuration represent the time each step takes to iterate one pixel for each respective thread.
             * 
             * The duration for each step is calculated by dividing the total animation duration by the absolute value of the lengt on the respective animation.
             * This ensures that each thread will iterate each pixel from start to end value,
             * ensuring that all threads will complete at the same time.
             */
            XAnimDuration = static_cast<const double &>(duration) / static_cast<const double &>(std::abs(endX - startX));
            GAnimDuration = frameDuration;

            /* START ANIMATION THREADS */
            GAnimationThread = std::thread(&Mwm_Animator::GFrameAnimation_X, this, endX);
            XAnimationThread = std::thread(&Mwm_Animator::CliXAnimation, this, endX);

            /* WAIT FOR ANIMATION TO COMPLETE */
            std::this_thread::sleep_for(std::chrono::milliseconds(duration));

            /* STOP THE ANIMATION */
            stopAnimations();
        }
    ;
    private: // variabels
        xcb_window_t window;
        client * c;
        std::thread GAnimationThread;
        std::thread XAnimationThread;
        std::thread YAnimationThread;
        std::thread WAnimationThread;
        std::thread HAnimationThread;
        int currentX;
        int currentY;
        int currentWidth;
        int currentHeight;
        int stepX;
        int stepY;
        int stepWidth;
        int stepHeight;
        double GAnimDuration;
        double XAnimDuration;
        double YAnimDuration;
        double WAnimDuration;
        double HAnimDuration;
        std::atomic<bool> stopGFlag{false};
        std::atomic<bool> stopXFlag{false};
        std::atomic<bool> stopYFlag{false};
        std::atomic<bool> stopWFlag{false};
        std::atomic<bool> stopHFlag{false};
        std::chrono::high_resolution_clock::time_point XlastUpdateTime;
        std::chrono::high_resolution_clock::time_point YlastUpdateTime;
        std::chrono::high_resolution_clock::time_point WlastUpdateTime;
        std::chrono::high_resolution_clock::time_point HlastUpdateTime;
        const double frameRate = 120;
        const double frameDuration = 1000.0 / frameRate; 
    ;
    private: // funcrions
        void XAnimation(const int & endX) /**
         *
         * @brief Performs animation on window 'x' position until the specified 'endX' is reached.
         * 
         * This function updates the 'x' of a window in a loop until the current 'x'
         * matches the specified end 'x'. It uses the "XStep()" function to incrementally
         * update the 'x' and the "thread_sleep()" function to introduce a delay between updates.
         * 
         * @param endX The desired 'x' position of the window.
         *
         */
        {
            XlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) 
            {
                if (currentX == endX) 
                {
                    config_window(XCB_CONFIG_WINDOW_X, endX);
                    break;
                }
                XStep();
                thread_sleep(XAnimDuration);
            }
        }
        void XStep() /**
         * @brief Performs a step in the X direction.
         * 
         * This function increments the currentX variable by the stepX value.
         * If it is time to render, it configures the window's X position using the currentX value.
         * 
         * @note This function assumes that the connection and window variables are properly initialized.
         */
        {
            currentX += stepX;
            
            if (XisTimeToRender())
            {
                xcb_configure_window
                (
                    conn,
                    window,
                    XCB_CONFIG_WINDOW_X,
                    (const uint32_t[1])
                    {
                        static_cast<const uint32_t &>(currentX)
                    }
                );
                xcb_flush(conn);
            }
        }
        void YAnimation(const int & endY) /**
         *
         * @brief Performs animation on window 'y' position until the specified 'endY' is reached.
         * 
         * This function updates the 'y' of a window in a loop until the current 'y'
         * matches the specified end 'y'. It uses the "YStep()" function to incrementally
         * update the 'y' and the "thread_sleep()" function to introduce a delay between updates.
         * 
         * @param endY The desired 'y' positon of the window.
         *
         */
        {
            YlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) 
            {
                if (currentY == endY) 
                {
                    config_window(XCB_CONFIG_WINDOW_Y, endY);
                    break;
                }
                YStep();
                thread_sleep(YAnimDuration);
            }
        }
        void YStep() /**
         * @brief Performs a step in the Y direction.
         * 
         * This function increments the currentY variable by the stepY value.
         * If it is time to render, it configures the window's Y position using xcb_configure_window
         * and flushes the connection using xcb_flush.
         */
        {
            currentY += stepY;
            
            if (YisTimeToRender())
            {
                xcb_configure_window
                (
                    conn,
                    window,
                    XCB_CONFIG_WINDOW_Y,
                    (const uint32_t[1])
                    {
                        static_cast<const uint32_t &>(currentY)
                    }
                );
                xcb_flush(conn);
            }
        }
        void WAnimation(const int & endWidth) /**
         *
         * @brief Performs a 'width' animation until the specified end 'width' is reached.
         * 
         * This function updates the 'width' of a window in a loop until the current 'width'
         * matches the specified end 'width'. It uses the "WStep()" function to incrementally
         * update the 'width' and the "thread_sleep()" function to introduce a delay between updates.
         * 
         * @param endWidth The desired 'width' of the window.
         *
         */
        {
            WlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) 
            {
                if (currentWidth == endWidth) 
                {
                    config_window(XCB_CONFIG_WINDOW_WIDTH, endWidth);
                    break;
                }
                WStep();
                thread_sleep(WAnimDuration);
            }
        }
        void WStep() /**
         *
         * @brief Performs a step in the width calculation and updates the window width if it is time to render.
         * 
         * This function increments the current width by the step width. If it is time to render, it configures the window width
         * using the XCB library and flushes the connection.
         *
         */
        {
            currentWidth += stepWidth;

            if (WisTimeToRender())
            {
                xcb_configure_window
                (
                    conn,
                    window,
                    XCB_CONFIG_WINDOW_WIDTH,
                    (const uint32_t[1])
                    {
                        static_cast<const uint32_t &>(currentWidth) 
                    }
                );
                xcb_flush(conn);
            }
        }
        void HAnimation(const int & endHeight) /**
         *
         * @brief Performs a 'height' animation until the specified end 'height' is reached.
         * 
         * This function updates the 'height' of a window in a loop until the current 'height'
         * matches the specified end 'height'. It uses the "HStep()" function to incrementally
         * update the 'height' and the "thread_sleep()" function to introduce a delay between updates.
         * 
         * @param endWidth The desired 'height' of the window.
         *
         */
        {
            HlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) 
            {
                if (currentHeight == endHeight) 
                {
                    config_window(XCB_CONFIG_WINDOW_HEIGHT, endHeight);
                    break;
                }
                HStep();
                thread_sleep(HAnimDuration);
            }
        }
        void HStep() /**
         *
         * @brief Increases the current height by the step height and updates the window height if it's time to render.
         * 
         * This function is responsible for incrementing the current height by the step height and updating the window height
         * if it's time to render. It uses the xcb_configure_window function to configure the window height and xcb_flush to
         * flush the changes to the X server.
         *
         */
        {
            currentHeight += stepHeight;
            
            if (HisTimeToRender())
            {
                xcb_configure_window
                (
                    conn,
                    window,
                    XCB_CONFIG_WINDOW_HEIGHT,
                    (const uint32_t[1])
                    {
                        static_cast<const uint32_t &>(currentHeight)
                    }
                );
                xcb_flush(conn);
            }
        }
        void GFrameAnimation(const int & endX, const int & endY, const int & endWidth, const int & endHeight)
        {
            while (true)
            {
                if (currentX == endX && currentY == endY && currentWidth == endWidth && currentHeight == endHeight)
                {
                    c->x_y_width_height(currentX, currentY, currentWidth, currentHeight);
                    break;
                }
                c->x_y_width_height(currentX, currentY, currentWidth, currentHeight);
                thread_sleep(GAnimDuration);
            }
        }
        void GFrameAnimation_X(const int & endX)
        {
            while (true)
            {
                if (currentX == endX)
                {
                    conf_client_x();
                    break;
                }
                conf_client_x();
                thread_sleep(GAnimDuration);
            }
        }
        void CliXAnimation(const int & endX) /**
         *
         * @brief Performs animation on window 'x' position until the specified 'endX' is reached.
         * 
         * This function updates the 'x' of a window in a loop until the current 'x'
         * matches the specified end 'x'. It uses the "XStep()" function to incrementally
         * update the 'x' and the "thread_sleep()" function to introduce a delay between updates.
         * 
         * @param endX The desired 'x' position of the window.
         *
         */
        {
            XlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) 
            {
                if (currentX == endX) 
                {
                    break;
                }
                currentX += stepX;
                thread_sleep(XAnimDuration);
            }
        }
        void CliYAnimation(const int & endY) /**
         *
         * @brief Performs animation on window 'y' position until the specified 'endY' is reached.
         * 
         * This function updates the 'y' of a window in a loop until the current 'y'
         * matches the specified end 'y'. It uses the "YStep()" function to incrementally
         * update the 'y' and the "thread_sleep()" function to introduce a delay between updates.
         * 
         * @param endY The desired 'y' positon of the window.
         *
         */
        {
            YlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) 
            {
                if (currentY == endY) 
                {
                    break;
                }
                currentY += stepY;
                thread_sleep(YAnimDuration);
            }
        }
        void CliWAnimation(const int & endWidth) /**
         *
         * @brief Performs a 'width' animation until the specified end 'width' is reached.
         * 
         * This function updates the 'width' of a window in a loop until the current 'width'
         * matches the specified end 'width'. It uses the "WStep()" function to incrementally
         * update the 'width' and the "thread_sleep()" function to introduce a delay between updates.
         * 
         * @param endWidth The desired 'width' of the window.
         *
         */
        {
            WlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) 
            {
                if (currentWidth == endWidth) 
                {
                    break;
                }
                currentWidth += stepWidth;
                thread_sleep(WAnimDuration);
            }
        }
        void CliHAnimation(const int & endHeight) /**
         *
         * @brief Performs a 'height' animation until the specified end 'height' is reached.
         * 
         * This function updates the 'height' of a window in a loop until the current 'height'
         * matches the specified end 'height'. It uses the "HStep()" function to incrementally
         * update the 'height' and the "thread_sleep()" function to introduce a delay between updates.
         * 
         * @param endWidth The desired 'height' of the window.
         *
         */
        {
            HlastUpdateTime = std::chrono::high_resolution_clock::now();
            while (true) 
            {
                if (currentHeight == endHeight) 
                {
                    break;
                }
                currentHeight += stepHeight;
                thread_sleep(HAnimDuration);
            }
        }
        void stopAnimations() /**
         *
         * @brief Stops all animations by setting the corresponding flags to true and joining the animation threads.
         *        After joining the threads, the flags are set back to false.
         *
         */
        {
            stopHFlag.store(true);
            stopXFlag.store(true);
            stopYFlag.store(true);
            stopWFlag.store(true);
            stopHFlag.store(true);

            if (GAnimationThread.joinable()) 
            {
                GAnimationThread.join();
                stopGFlag.store(false);
            }

            if (XAnimationThread.joinable()) 
            {
                XAnimationThread.join();
                stopXFlag.store(false);
            }

            if (YAnimationThread.joinable()) 
            {
                YAnimationThread.join();
                stopYFlag.store(false);
            }

            if (WAnimationThread.joinable()) 
            {
                WAnimationThread.join();
                stopWFlag.store(false);
            }

            if (HAnimationThread.joinable()) 
            {
                HAnimationThread.join();
                stopHFlag.store(false);
            }
        }
        void thread_sleep(const double & milliseconds) /**
         *
         * @brief Sleeps the current thread for the specified number of milliseconds.
         *
         * @param milliseconds The number of milliseconds to sleep. A double is used to allow for
         *                     fractional milliseconds, providing finer control over animation timing.
         *
         * @note This is needed as the time for each thread to sleep is the main thing to be calculated, 
         *       as this class is designed to iterate every pixel and then only update that to the X-server at the given framerate.
         *
         */
        {
            // Creating a duration with double milliseconds
            auto duration = std::chrono::duration<double, std::milli>(milliseconds);

            // Sleeping for the duration
            std::this_thread::sleep_for(duration);
        }
        bool XisTimeToRender() /**
         *
         * Checks if it is time to render based on the elapsed time since the last update.
         * @return true if it is time to render, @return false otherwise.
         *
         */
        {
            // CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
            const auto & currentTime = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<double, std::milli> & elapsedTime = currentTime - XlastUpdateTime;

            if (elapsedTime.count() >= frameDuration) 
            {
                XlastUpdateTime = currentTime; 
                return true; 
            }
            return false; 
        }
        bool YisTimeToRender() /**
         *
         * Checks if it is time to render a frame based on the elapsed time since the last update.
         * @return true if it is time to render, @return false otherwise.
         *
         */
        {
            // CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
            const auto & currentTime = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<double, std::milli> & elapsedTime = currentTime - YlastUpdateTime;

            if (elapsedTime.count() >= frameDuration) 
            {
                YlastUpdateTime = currentTime; 
                return true; 
            }
            return false; 
        }
        bool WisTimeToRender() /**
         *
         * Checks if it is time to render based on the elapsed time since the last update.
         * @return true if it is time to render, @return false otherwise.
         *
         */
        {
            // CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
            const auto & currentTime = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<double, std::milli> & elapsedTime = currentTime - WlastUpdateTime;

            if (elapsedTime.count() >= frameDuration) 
            {
                WlastUpdateTime = currentTime; 
                return true; 
            }
            return false; 
        }
        bool HisTimeToRender() /**
         *
         * Checks if it is time to render based on the elapsed time since the last update.
         * @return true if it is time to render, @return false otherwise.
         *
         */
        {
            // CALCULATE ELAPSED TIME SINCE THE LAST UPDATE
            const auto & currentTime = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<double, std::milli> & elapsedTime = currentTime - HlastUpdateTime;

            if (elapsedTime.count() >= frameDuration) 
            {
                HlastUpdateTime = currentTime; 
                return true; 
            }
            return false; 
        }
        void config_window(const uint32_t & mask, const uint32_t & value) /**
         *
         * @brief Configures the window with the specified mask and value.
         * 
         * This function configures the window using the XCB library. It takes in a mask and a value
         * as parameters and applies the configuration to the window.
         * 
         * @param mask The mask specifying which attributes to configure.
         * @param value The value to set for the specified attributes.
         * 
         */
        {
            xcb_configure_window
            (
                conn,
                window,
                mask,
                (const uint32_t[1])
                {
                    static_cast<const uint32_t &>(value)
                }
            );
            xcb_flush(conn);
        }
        void config_window(const xcb_window_t & win, const uint32_t & mask, const uint32_t & value) /**
         *
         * @brief Configures the window with the specified mask and value.
         * 
         * This function configures the window using the XCB library. It takes in a mask and a value
         * as parameters and applies the configuration to the window.
         * 
         * @param mask The mask specifying which attributes to configure.
         * @param value The value to set for the specified attributes.
         * 
         */
        {
            xcb_configure_window
            (
                conn,
                win,
                mask,
                (const uint32_t[1])
                {
                    static_cast<const uint32_t &>(value)
                }
            );
            xcb_flush(conn);
        }
        void config_client(const uint32_t & mask, const uint32_t & value) /**
         *
         * @brief Configures the window with the specified mask and value.
         * 
         * This function configures the window using the XCB library. It takes in a mask and a value
         * as parameters and applies the configuration to the window.
         * 
         * @param mask The mask specifying which attributes to configure.
         * @param value The value to set for the specified attributes.
         * 
         */
        {
            xcb_configure_window
            (
                conn,
                c->win,
                mask,
                (const uint32_t[1])
                {
                    static_cast<const uint32_t &>(value)
                }
            );

            xcb_configure_window
            (
                conn,
                c->frame,
                mask,
                (const uint32_t[1])
                {
                    static_cast<const uint32_t &>(value)
                }
            );
            xcb_flush(conn);
        }
        void conf_client_x()
        {
            const uint32_t x = currentX;
            c->frame.x(x);
            xcb_flush(conn);
        }
    ;
};

#endif // MWM_ANIMATOR_HPP