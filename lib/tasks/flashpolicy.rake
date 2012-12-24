namespace :flashpolicy do
  task :start do
    daemon_path = "#{Rails.root}/src/flashpolicyd"
    policy_xml = "#{daemon_path}/flashpolicy.xml"
    command = "sudo #{daemon_path}/flashpolicyd.pl --file=#{policy_xml} --port=843"
    puts 'command> ' + command
    system(command)
  end

end
