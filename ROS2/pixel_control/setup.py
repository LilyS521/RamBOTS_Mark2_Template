
from setuptools import find_packages, setup

package_name = 'pixel_control'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        (
            'share/ament_index/resource_index/packages',
            ['resource/' + package_name],
        ),
        (
            'share/' + package_name,
            ['package.xml'],
        ),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='jro2003',
    maintainer_email='jacobrolguin@gmail.com',
    description='ROS 2 movement command publisher and subscriber for Pixel.',
    license='Apache-2.0',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'movement_publisher = pixel_control.movement_publisher:main',
            'movement_subscriber = pixel_control.movement_subscriber:main',
        ],
    },
)
